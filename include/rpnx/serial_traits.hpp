// Copyright (c) 2016, 2017, 2018 Ryan P. Nicholl <exaeta@protonmail.com>; All Rights Reserved
// this is probably not the most beautiful it could be


#ifndef RPNX_SERIAL_TRAITS_HH
#define RPNX_SERIAL_TRAITS_HH

#include <inttypes.h>
#include <iostream>
#include <tuple>
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <type_traits>
#include <vector>
#include <cstdint>

namespace rpnx
{
  class uintany;
  class intany;

  template <typename It>
  class container_extractor
    : public It
  {
  public:
    container_extractor() = delete;

    static typename It::container_type* extract(It const& it)
    {
      return it.container;
    }
  };

  template <typename It>
  auto extract_container(It const&from) -> decltype(container_extractor<It>::extract(from))
  {
    return container_extractor<It>::extract(from);
  }

  class asn_counter
  {
  public:
    size_t count;
  
    class proxy
    {
      asn_counter & ref;
    public:
      proxy(asn_counter &r) : ref(r) {}

      template <typename T>
      proxy& operator=(T const &)
      {
        ref.count++;
        return *this;
      }
    };

    asn_counter& operator++()
    {
      return *this;
    }

    asn_counter& operator++(int)
    {
      return *this;
    }

    proxy operator*() 
    {
      return proxy(*this);
    }


    asn_counter()
      : count(0)
    {
    }
  
    asn_counter(asn_counter const &)=default;  
    asn_counter& operator = (asn_counter const &)=default;
  };



  /*
    Serial traits base cases.

    0 - no base case (user defined, or does not exist)
    1 - unsigned integral
    2 - signed integral
    3 - vector-like
    4 - tuple-like
    5 - map-like
    6 - string-like
    7 - reference

  */
  template <typename T>
  struct serial_traits_base_cases
  {
    serial_traits_base_cases() = delete;

    static constexpr int base_case()
    {
      if (std::is_const<T>::value || std::is_reference<T>::value) return 0;
      if (std::is_integral<T>::value && std::is_unsigned<T>::value)
        {
          return 1;
        }

      if (std::is_integral<T>::value && std::is_signed<T>::value)
        {
          return 2;
        }


      return 0;
    }
  };




  template <typename ... Ts>
  struct serial_traits_base_cases<std::vector<Ts...> >
  {
    static constexpr int base_case() { return 3; }
  };

  template <typename ... Ts>
  struct serial_traits_base_cases<std::tuple<Ts...>>
  {
    static constexpr int base_case() { return 4; }
  };

  template <typename Ts, size_t N>
  struct serial_traits_base_cases<std::array<Ts, N>>
  {
    static constexpr int base_case() { return 4; }
  }
    ;

  template <typename ... Ts>
  struct serial_traits_base_cases<std::pair<Ts...>>
  {
    static constexpr int base_case() { return 4; }
  };

  template <typename ... Ts>
  struct serial_traits_base_cases<std::basic_string<Ts...>>
  {
    static constexpr int base_case() { return 3; }
  };

  template <typename ... Ts>
  struct serial_traits_base_cases<std::map<Ts...>>
  {
    static constexpr int base_case() { return 5; }
  };

  template <typename T>
  struct serial_traits_base_cases<T &>
  {
    static constexpr int base_case() { return 7; }
  };





  template <typename T, int C = serial_traits_base_cases<T>::base_case()>
  struct serial_traits;
  template <>
  struct serial_traits<uint8_t, 1>;
  template <typename T>
  struct serial_traits<T, 0>;
  template <typename T>
  struct serial_traits<T,1>;



  template <typename T>
  struct serial_traits<T, 0>
  {
    static void dev_test()  { std::cout << "serial_traits(undefined for this type)" << std::endl; }  
  };



  template <typename T>
  class has_noarg_serial_size_helper
  {
    template <typename C> static std::false_type test(...);
    template <typename C> static std::true_type test(decltype(serial_traits<C>::serial_size()));
  public:
    using type = decltype(test<T>(0));
  };

  template <typename T>
  class has_noarg_serial_size
    : public  has_noarg_serial_size_helper<T>::type
  {
  };


  template <typename T>
  struct serial_traits_defaults
  {
    static size_t serial_size(T const &t)
    {
      asn_counter it;

      it = serial_traits<T>::serialize(t, it);
    
      return it.count;
    }
  };

  template <>
  struct serial_traits<uint8_t, 1>
  {
    static constexpr size_t serial_size(uint8_t const &) { return 1; }
    static constexpr size_t serial_size() { return 1; }

    static constexpr bool serial_size_constexpr() { return true; }

    template <typename It>
    static auto serialize(uint8_t const & in, It out) -> It
    {
      *out++ = in;
      return out;
    }

    template <typename It>
    static auto deserialize(uint8_t & out, It in) -> It
    {
      out = *in++;
      return in;
    }

  
    static void dev_test()  { std::cout << "serial_traits(uint8_t)" << std::endl; }
  };




  template <>
  struct serial_traits<uintany, 0>
  {
 

    template <typename It>
    static constexpr auto serialize (uintmax_t const & in, It out) -> It
    {
      uintmax_t base = in;

      uintmax_t bytecount = 1;
      uintmax_t max = 0;

      max = (1ull << (7))-1;
      while (base > max)
        {
          bytecount++;
          base -= max+1;
          max = (1ull << ((7)*bytecount))-1;     
        }
    
      for (uintmax_t i = 0; i < bytecount; i++)
        {
          uint8_t val = base & ((uintmax_t(1) << (7)) -1);
          if (i != bytecount-1)
            {
              val |= 0b10000000;
            }
          *out++ = val;
          base >>= 7;
        }
      return out;   
    
    }

    template <typename It>
    static constexpr auto deserialize(uintmax_t  & n, It in ) -> It
    {
      n = 0;
      uintmax_t n2 = 0;


      while (true)
        {
          uint8_t a = *in++;

          uint8_t read_value = a & 0b1111111;

          n += (uintmax_t(read_value) << (n2*7));

          if (!(a&0b10000000)) break;
          n2++;
        }

      for (uintmax_t i = 1; i <= n2; i++)
        {
          n += (uintmax_t(1) << (i*7));
        }
      return in;
    }

    class async_deserializer
    {
      std::uintmax_t n1;
      std::uintmax_t n2;
      bool b1;
    public:
      async_deserializer()
        : n1(0), n2(0), b1(false)
      {
      }

      async_deserializer(async_deserializer const &)=default;
      async_deserializer(async_deserializer &&)=default;

      void reset()
      {
        n1 = 0;
        n2 = 0;
        b1 = false;
      }

      bool ready() const
      {
        return b1;
      }

      /** insert(1) Add a byte to the deserializer
          @precondition ready() == false
          @postcondition ready() == <return_value>
          @param a The input byte
          @returns bool: The value of ready()
      */
      bool insert(uint8_t a)
      {
        if (ready()) __builtin_unreachable();
        uint8_t r = a & 0b1111111;

        n1 += (uintmax_t(r) << (n2*7));
      
        bool more = a&0b10000000;
        if (!more) 
          {
            for (uintmax_t i = 1; i <= n2; i++)
              {
                n1 += (uintmax_t(1) << (i*7));
              }
            b1 = true;
            return true;
          }
        else 
          {
            n2++;
            return false;
          }
      }

      /** insert(2) Add a range of bytes to the deserializer
          @precondition ready()==false
          @returns A std::pair<It, bool> .first is the end of the added range. .second indicates the resulting state of ready()
          (Note: If .second is false then .first always equals the end iterator, this function will always consume as many bytes as possible unless an exception occurs)
      */
      template<typename It>
      auto insert(It begin, It end) -> It
      {
        if (ready()) __builtin_unreachable();
        auto it = begin;
        while (it != end && !ready())
          {
            insert(*it);
            it++;
          }
        if (it != end && !ready()) __builtin_unreachable();
        return {it, ready()};
      }

      /** Returns the output
          If ready()==false when this function is called, the behavior is undefined.
          If this function is called twice, the behavior is undefined.
          To reuse this deserializer, call reset() first
      */
      uintmax_t get()
      {
        if (!ready()) __builtin_unreachable();
        uintmax_t t = n1;
        reset();
        return t;
      }

      /** Returns the minimum amount of additional data required that is known to be required at this time.
          This function is only valid if ready()==false.
      */
      size_t more_min() const
      {
        if (ready()) __builtin_unreachable();
        if (ready()) return 0;
        else return 1;
      }

      /** Returns the maximum amount of additional data known to be required at this time
       */
      size_t more_max() const
      {
        return SIZE_MAX;
      }
  

    };

    static size_t serial_size(size_t const & t)
    {
      asn_counter iterator;

      iterator = serialize(t, iterator);

      return iterator.count;
    }

  };

  /*
    The intany class gives well defined behavior for conversion of signed values to unsigned values.
    The conversion is slightly different from twos compliment because the conversion needs to be able 
    to deal with variable size values in a sensible way.

  */


  template <>
  struct serial_traits<intany, 0>
  {

  private:
    static inline uintmax_t itou(intmax_t in)
    {
      uintmax_t v = 0;
    
      if (in >= 0)
        {
          v   = in;
          v <<= 1;
        }
      else
        {
          v   = -in;
          v  -= 1;
          v <<= 1;
          v  |= 1;
        }
      return v;
    }


    static inline intmax_t utoi(uintmax_t v)
    {
      intmax_t n;
      if ((v & 1) == 0)
        {
          v >>= 1;
          n = v;
        }
      else
        {
          v >>=  1;
          v  +=  1;
          n   = -v;
        }
      return n;
    }

  public:

    class async_deserializer
    {
      serial_traits<uintany>::async_deserializer a;
    public:
      intmax_t get()
      {
        return utoi(a.get());
      }

      bool ready()
      {
        return a.ready();
      }

      void reset()
      {
        a.reset();
      }

      template <typename It>
      auto insert(It b, It e) -> std::pair<It, bool>
      {
        return a.insert(b, e);
      }

      bool insert(uint8_t c)
      {
        return a.insert(c);
      }

        
      size_t more_min() const
      {
        return a.more_min();
      }

      size_t more_max() const
      {
        return a.more_max();
      }
    };

    // made public for debugging only, don't use
  

    template <typename It>
    static constexpr auto serialize (ssize_t const & in, It out) -> It
    {
      out = serial_traits<uintany>::serialize(itou(in), out);
      return out;    
    }

    template <typename It>
    static constexpr auto deserialize(ssize_t  & n, It in ) -> It
    {
      size_t v=0;

      in = serial_traits<uintany>::deserialize(v, in);

      n = utoi(v);
      return in;
    }

    static size_t serial_size(size_t const & t)
    {
      asn_counter iterator;

      iterator = serialize(t, iterator);

      return iterator.count;
    }

  };


  template <typename T>
  struct serial_traits<T,1>
  {
    static constexpr size_t serial_size(T const &) { return sizeof(T); }
    static constexpr size_t serial_size() { return sizeof(T); }
    static constexpr bool serial_size_constexpr() { return true; }

    template <typename It>
    static auto serialize(T const & in, It out) -> It
    {
      T tm = in;
      for (size_t i = 0; i < serial_size(); i++)
        {
          out = serial_traits<uint8_t>::serialize(tm & 0xFF, out);
          tm >>= 8;
        }
      return out;
    }

    template <typename It>
    static constexpr auto deserialize(T & out, It in) -> It
    {
      out = 0;
      for (size_t i = 0; i < serial_size(); i++)
        {
          uint8_t value = 0;
          in = serial_traits<uint8_t>::deserialize(value, in);
          out |= (T(value) << (8*i));
        }
      return in;
    }

    class async_deserializer
    {
      T a;
      size_t i;
    public:
      async_deserializer()
      {
        reset();
      }

      void reset()
      {
        a = 0;
        i = 0;
      }

      T get()
      {
        T t = a;
        reset();
        return t;
      }
  
      bool insert(uint8_t c)
      {
        uint8_t value = c;
        a |= T(value) << (8*i);
        i++;
        return ready();
      }
  
      template<typename It>
                         auto insert(It begin, It end) -> It
      {
        auto it = begin;
        while (it != end && !ready())
          {
            insert(*it);
            it++;
          }
        return {it, ready()};
      }
  

  
      bool ready() const
      {
        return i==serial_size();
      }
    };

    static void dev_test()  { std::cout << "serial_traits(unsigned integral)" << std::endl; }
  };


  template <typename T>
  struct serial_traits<T&, 7>
    : public serial_traits< typename std::remove_reference<T>::type>
  {
  };

  template <typename T>
  struct serial_traits<T,2>
  {
    static constexpr size_t serial_size(T const &) { return sizeof(T); }
    static constexpr size_t serial_size() { return sizeof(T); }
    static constexpr bool serial_size_constexpr() { return true; }

    template <typename It>
    static auto serialize(T const & in, It out) -> It
    {
      T tm = in;
      for (size_t i = 0; i < serial_size(); i++)
        {
          out = serial_traits<uint8_t>::serialize(tm & 0xFF, out);
          tm >>= 8;
        }
      return out;
    }

  
    template <typename It>
    static constexpr auto deserialize(T & out, It in) -> It
    {
      out = 0;
      for (size_t i = 0; i < serial_size(); i++)
        {
          uint8_t value = 0;
          in = serial_traits<uint8_t>::deserialize(value, in);
          out |= (value << (8*i));
        }
      return in;
    
    }

    class async_deserializer
    {
      T a;
      size_t i;
    public:
      async_deserializer()
      {
        reset();
      }

      void reset()
      {
        a = 0;
        i = 0;
      }

      T get()
      {
        if (!ready()) __builtin_unreachable();
        T t = a;
        reset();
        return t;
      }

      bool insert(uint8_t c)
      {
        uint8_t value = c;
        a |= T(value) << (8*i);
        i++;
        return ready();
      }

      template<typename It>
                         auto insert(It begin, It end) -> It
      {
        if (ready()) __builtin_unreachable();
        auto it = begin;
        while (it != end && !ready())
          {
            insert(*it);
            it++;
          }
        if (it != end && !ready()) __builtin_unreachable();
        return {it, ready()};
      }

      bool ready() const
      {
        return i==serial_size();
      }
    
    };
  

    static void dev_test()  { std::cout << "serial_traits(signed integral)" << std::endl; }
  };

  template <typename T, bool B = has_noarg_serial_size<typename T::value_type>::value >
  struct vector_size_helper;

  template <typename T>
  struct vector_size_helper<T, false>
  {
    static size_t serial_size(T const &what)
    {
      size_t sz = serial_traits<uintany>::serial_size(what.size());
      //    std::cerr << "case vector<non-const size>" << std::endl;
      for (auto const & x: what)
        {
          sz += serial_traits<typename T::value_type>::serial_size(x);
        }
      return sz;
    }
  };


  template <typename T>
  struct vector_size_helper<T, true>
  {
    static inline size_t serial_size(T const &what)
    {
      size_t sz = serial_traits<uintany>::serial_size(what.size());
      //    std::cerr << "case vector<const size>" << std::endl;
      sz += what.size()*serial_traits<typename T::value_type>::serial_size();
      return sz;
    }
  };


  template <typename T>
  struct serial_traits<T, 3>
  {
    static void dev_test()  { std::cout << "serial_traits(vector-like)" << std::endl; }

    static constexpr bool serial_size_constexpr() { return false; }

    static size_t serial_size(T const & what)
    {
      return vector_size_helper<T>::serial_size(what);
    }


    template <typename It>
    static auto serialize(T const & in, It out) -> It
    {
      out = serial_traits<uintany>::serialize(in.size(), out);

      for (auto it = begin(in); it != end(in); it++)
        {
          out = serial_traits<typename T::value_type>::serialize(*it, out);
        }
    
      return out;
    }
  
    template <typename It>
    static auto deserialize(T & out, It in) -> It
    {
      size_t count;
      in = serial_traits<uintany>::deserialize(count, in);
      for (size_t i = 0; i < count; i++)
        {
          typename T::value_type t;
          in = serial_traits<typename T::value_type>::deserialize(t, in);
          out.push_back(std::move(t));
        }


      return in;
    }

    class async_deserializer
    {
      T out;
      typename serial_traits<uintany>::async_deserializer szd;
      typename serial_traits<typename T::value_type>::async_deserializer td;
      size_t sz;
      size_t i;
    
      int stage;
    public:
      void reset()
      {
        stage = 0;
        sz = 0;
        i = 0;
        out = T();
      }

      bool ready() const
      {
        if (stage == 0) return false;
        if (stage == 1 && i == sz) return true;
      }

      bool insert(uint8_t c)
      {
        if (stage == 0)
          {
            szd.insert(c);
            if (szd.ready())
              {
                sz = szd.get();
                stage++;
              }
            return ready();
          }
        if (stage == 1)
          {
            if (td.insert(c))
              {
                i++;
                td.reset();
              }
            return ready();
          }
      }
    };
  };


  template <typename T, bool S1 = has_noarg_serial_size<typename T::key_type>::value, bool S2 = has_noarg_serial_size<typename T::mapped_type>::value>
  struct map_size_helper;

  template <typename T>
  struct map_size_helper<T, false, false>
  {
    static size_t serial_size(T const & what)
    {
      size_t sz = 0;
      for (auto it = what.begin(); it != what.end(); it++)
        {
          sz += serial_traits<typename T::key_type>::serial_size(it->first);
          sz += serial_traits<typename T::mapped_type>::serial_size(it->second);
        }
      return sz;
    
    }
  };

  template <typename T>
  struct map_size_helper<T, true, false>
  {
    static size_t serial_size(T const & what)
    {
      size_t sz = 0;
    
      sz += what.size()*serial_traits<typename T::key_type>::serial_size();
      for (auto it = what.begin(); it != what.end(); it++)
        {       
          sz += serial_traits<typename T::mapped_type>::serial_size(it->second);
        }

      return sz;
    }
  };

  template <typename T>
  struct map_size_helper<T, false, true>
  {
    static size_t serial_size(T const & what)
    {
      size_t sz = 0;
    
      sz += what.size()*serial_traits<typename T::mapped_type>::serial_size();
      for (auto it = what.begin(); it != what.end(); it++)
        {       
          sz += serial_traits<typename T::key_type>::serial_size(it->first);
        }

      return sz;
    }
  };

  template <typename T>
  struct map_size_helper<T, true, true>
  {
    static size_t serial_size(T const & what)
    {
      size_t sz = 0;
    
      sz += what.size()*serial_traits<typename T::mapped_type>::serial_size();
      sz += what.size()*serial_traits<typename T::key_type>::serial_size();
      return sz;

    }
  };



  template <typename T>
  struct serial_traits<T, 5>
  {
    static void dev_test()  { std::cout << "serial_traits(map)" << std::endl; }

  private:

  
  public:
    static size_t serial_size(T const & what)
    {
      size_t sz = serial_traits<uintany>::serial_size(what.size());
      sz += map_size_helper< T >::serial_size(what);
      return sz;
    }
  
    static constexpr bool serial_size_constexpr() { return false; }
    template <typename It>
    static auto serialize(T const & in, It out) -> It
    {
      out = serial_traits<uintany>::serialize(in.size(), out);

      for (auto it = begin(in); it != end(in); it++)
        {
          out = serial_traits<typename T::value_type>::serialize(*it, out);
        }
    
      return out;
    }

    template <typename It>
    static auto deserialize(T & out, It in) -> It
    {
      out.clear();
      size_t sz = 0;
      in = serial_traits<uintany>::deserialize(sz, in);

      for (size_t i = 0; i < sz; i++)
        {
          std::pair<typename T::key_type, typename T::mapped_type> iv;
          in = serial_traits<decltype(iv)>::deserialize(iv, in);
          out.insert(std::move(iv));
        }
      return in;
    }
  };



  template <typename T, int I = 0, bool last = (std::tuple_size<T>::value-1 == I)>
  struct tuple_serial_traits;

  template <typename T, int I>
  struct tuple_serial_traits<T, I, true>
  {
    static constexpr bool serial_size_constexpr() 
    {
      return serial_traits<typename std::tuple_element<I, T>::type>::serial_size_constexpr;
    }
    template <typename It>
    static auto serialize(T const & in, It out) -> It
    {
      return serial_traits<typename std::tuple_element<I, T>::type>::serialize(std::get<I>(in), out);
    }

    template <typename It>
    static auto deserialize(T & out, It in) -> It
    {
      return serial_traits<typename std::tuple_element<I, T>::type >::deserialize(std::get<I>(out), in);
    }
  };



  template <typename T, int I>
  struct tuple_serial_traits<T, I, false>
  {

    template <typename It>
    static auto serialize(T const & in, It out) -> It
    {
      out = serial_traits<typename std::tuple_element<I, T>::type>::serialize(std::get<I>(in), out);
      return tuple_serial_traits<T, I+1>::serialize(in, out);
    }
  
    template <typename It>
    static auto deserialize(T & out, It in) -> It
    {
      in = serial_traits<typename std::tuple_element<I, T>::type>::deserialize(std::get<I>(out), in);
      return tuple_serial_traits<T, I+1>::deserialize(out, in);
    }

  };


  template <typename T>
  struct serial_traits<T, 4>
  {
    static void dev_test()  { std::cout << "serial_traits(tuple)" << std::endl; }

    template <typename It>
    static auto serialize(T const & in, It out) -> It
    {
      return tuple_serial_traits<T>::serialize(in, out);
    }

    template <typename It>
    static auto deserialize(T & out, It in) -> It
    {
      return tuple_serial_traits<T>::deserialize(out, in);
    }

  };


  template <typename T>
  struct serial_traits<T const, 0>
  {
    template <typename It>
    static auto serialize(T const & in, It out) -> It
    {
      return serial_traits<T>::serialize(in, std::move(out));
    }
  };


  template <typename T, typename It>
  struct serial_helper
  {
    static auto serialize(T const & in, It out) -> It
    {
      return serial_traits<T>::serialize(in, out);
    }
    static auto deserialize(T  & out, It in) -> It
    {
      return serial_traits<T>::deserialize(out, in);
    }
  };
  

  template <template <typename> typename, typename>
  class tuple_converter;

  template <template <typename> typename Templ, typename ... Ts>
  class tuple_converter< Templ,  std::tuple<Ts...> >
  {
  public:
    using type = typename std::tuple<typename Templ<typename std::remove_reference<Ts>::type>::type...>;
  };


  template <typename T>
  struct tuple_converter_type_helper
  {
    using type = typename serial_traits<T>::async_deserialize;
  };


  // The following code requires serial_size to actually be implemented for all classes (it's not)
  /*
    template <typename T, typename A>
    struct serialize_helper<T, std::back_insert_iterator<std::vector<uint8_t, A> > >
    {
    static inline auto serialize(T const & in, std::back_insert_iterator<std::vector<uint8_t, A> > out) -> std::back_insert_iterator<std::vector<uint8_t, A> >
    {
    std::vector<uint8_t, A> & vec = *extract_container(out);
    vec.reserve(vec.size()+serial_traits<T>::serial_size(in));
    return serial_traits<T>::serialize(in, out);
    }
    };

  */

  template <typename T, typename It>
  auto serialize(T const & in, It out) -> It
  {
    return serial_helper<T, It>::serialize(in, out);
  }


  template <typename T, typename It>
  auto deserialize(T & out, It in) -> It
  {
    return serial_helper<T, It>::deserialize(out, in);
  }

}
#endif
