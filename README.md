# Abandoned

Repo is abandoned in favor of rpnx-core. No further updates will be made.

https://github.com/rpnx-net/rpnx-core

# rpnx::serial_traits

A work in progress library that provides serialization and deserialization routines for common types.

## TODO

  * Clean up code
  * Add async support

## Installing

```
git clone https://github.com/Exaeta/rpnx-serial.git &&
mkdir rpnx-serial.build &&
cd rpnx-serial.build &&
cmake ../rpnx-serial &&
sudo make install
``` 

## Using

```#include <rpnx/serial_traits>```

## Example usage

```c++
#include <rpnx/serial_traits>
#include <string>
#include <map>
#include <vector>
#include <iterator>
#include <bitset>
int main()
{
  using namespace rpnx;
  using namespace std;

  map<set<uint16_t>, string> example_object { {{1, 3, 4}, "hello"}, {{4, 5}, "world" }};
  
  map<set<uint16_t>, string> example_object2;
  
  vector<uint8_t> data_vec;
    
  serialize(example_object, back_inserter(data_vec));
  
  for (size_t i = 0; i < data_vec.size(); i++)
  {
    cout << std::bitset<8>(data_vec[i]);
    if (i != data_vec.size() -1) cout << " ";
  }
  
  cout << endl;
  cout << "example_object " << ( example_object == example_object2 ? "==" : "!=" ) << " example_object2" << endl;
  
  cout << "deserializing!" << endl;
  deserialize(example_object2, data_vec.begin());
  
  cout << "example_object " << ( example_object == example_object2 ? "==" : "!=" ) << " example_object2" << endl;
    
}
```
Outputs:
```
00000010 00000011 00000001 00000000 00000011 00000000 00000100 00000000 00000101 01101000 01100101 01101100 01101100 01101111 00000010 00000100 00000000 00000101 00000000 00000101 01110111 01101111 01110010 01101100 01100100
example_object != example_object2
deserializing!
example_object == example_object2

```
## Notes

When serializing values of type ```size_t```, ```ptrdiff_t```, and other variable width types, use the following functions:
```
rpnx::serial_traits<uintany>::serialize(...);
rpnx::serial_traits<uintany>::deserialize(...);
```
Failure to do so could cause your objects to have different binary formats when read/written on different platforms.

The deserializer does NOT perform bounds checking. Use a bounds checked iterator.

## Upcoming Version 2.0

The next version of the library will have a different API, and be more efficient.

The relevant overloads will be:


```C++
static constexpr size_t rpnx::serial_size<T>();
```
Returns the serial size. This member function is only present on types with fixed serial sizes. (e.g., uint64_t)


```C++
static constexpr size_t rpnx::serial_size<T>(T const & t);
```
Returns what the serialized size of the object ```t``` would be.


```C++
/*(1)*/ static constexpr auto rpnx::serialize(ItF && f, T const & t)->decltype(f());

```
Serialization functions.


Function ```(1)``` expects ```f(n)``` to be callable as though it was of type ```OIt(size_t N)``` where ```OIt``` is an Output Iterator which can accommodate exactly N bytes. The functor ```f``` must be callable repeatedly. Each sucessive call will specify the number of bytes to be output to the returned iterator. Bytes will be  written in the correct order. Each time ```f``` is called, iterators returned by ```f(...)``` previously need not remain valid. Thus, it is acceptable to e.g. have ```f(n)``` return a ```std::back_insert_iterator<...>```.

Function ```(2)``` expects an Output Iterator. You *must* ensure that the output iterator can accomodate the full size of the data structure. You could either check the serial size of the structure in advance, which will be exactly ```rpnx::serial_traits<T>::size(t)``` bytes, or you could use e.g. a ```std::back_insert_iterator<...>```</s>

```C++
/*(1)*/ static constexpr auto rpnx::deserialize(ItF && f, T & t)->decltype(f());
```
Functions for deserializing objects.

Function ```(1)``` expects ```f``` to be callable as if of type ```IIt(size_t N)``` where ```IIt``` is an Input Iterator that at least N bytes can be read from. You can throw an exception which will not be caught if you don't have enough input data. If you throw an exception from the ```f``` function, the target object is left in an unspecified but valid state. The function will read N bytes from that input iterator. ```f``` may be called any number of times.

### Overloading in upcoming 2.0

To define a custom class which is serializable, the following classes should be overridden:

```C++
template <>
class rpnx::serial_type_traits<T>
{
  /* Only define the following function if your class has a FIXED size when serialized.
     This function returns the serialized size of a member of your class.
   */
  static constexpr std::size_t size() { ... }
  
  /* Define this function ALWAYS. It is strongly recommended to make it constexpr if possible.
     This function returns the exact serialized size that an instance of your class would be.
   */
  static constexpr std::size_t size(T const & t) { ... }
};

template <typename ItF>
class rpnx::serial_functor_traits<T, ItF>
{
  /** This function should seralize the object t by requesting an output iterator from the function f.
      You can call f multiple times, but old iterators are invalidated by a call to f(...)   
   */
  static constexpr auto serialize(ItF && f, T const & t) -> It
  {
    ...
  }
  
  /** This function should deseralize the object t by requesting an input iterator from the function f.
      You can call f multiple times, but old iterators are invalidated by a call to f(...)   
   */
  static constexpr auto deserialize(ItF && f, T  & t) -> It
  {
    ...
  }
  
};

template <typename It>
class rpnx::serial_iterator_traits<T, It>
{
  /** The caller must guarantee that 'it' is an output iterator that can accept the required number of bytes.
   */
  static constexpr auto serialize_it(It it, T const & t) -> It
  {
    ...
  }
  
  /** The caller must guarantee that 'it' is an input iterator that can provide the required number of bytes.
      This function is VERY dangerous, and should only be used with iterators you know can accomodate the data,
      (such as those returned by f() in other functions).
     
      This function should be implemented for all types.
      
      e.g.:
      
        SAFE:
          * deserializing a fixed serial size type such std::tuple<uint64_t, uint64_t> when you know the 
            input buffer is 16 bytes long.
          * deserializing something that will be exactly N bytes to an iterator you received as the result
            of a call to f(N) from the argument to deserialize(ItF && f, T const & t)
          
        NOT SAFE:
          * deserializing a std::tuple<uint64_t, uint64_t> when you don't check the input size beforehand
          * deserializing a std::vector<uint64_t> recieved over the network using a vector's .begin()
          * deserializing a std::map<uint64_t> using a buffer's iterator
          * deserializing with a serial type of uintany or an intany an integer of type I from
            a buffer that might not be able to accomodate a unintany or intany value of std::numeric_limits<I>::max(),
            or std::numeric_limits<I>::min(), which will usually be larger than the sizeof that integral type. 
      
      Returns the advanced output iterator. 
   */
  static constexpr auto deserialize_it(It it, T  & t) -> It
  {
    ...
  }
  
};
```

To serialize an object, use ```rpnx::serialize(ItF && f, T const & object)```. Deserialization is done by ```rpnx::deserialize(ItF && f, T & object)```. The function ```rpnx::serialize_it(It it, T const & object)``` is also defined.

