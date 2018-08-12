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
    
  serial_traits<decltype(example_object)>::serialize(example_object, back_inserter(data_vec));
  
  for (size_t i = 0; i < data_vec.size(); i++)
  {
    cout << std::bitset<8>(data_vec[i]);
    if (i != data_vec.size() -1) cout << " ";
  }
  
  cout << endl;
  cout << "example_object " << ( example_object == example_object2 ? "==" : "!=" ) << " example_object2" << endl;
  
  cout << "deserializing!" << endl;
  serial_traits<decltype(example_object2)>::deserialize(example_object2, data_vec.begin());
  
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
