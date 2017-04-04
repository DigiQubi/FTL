// All content copyright (c) Allan Deutsch 2017. All rights reserved.
#pragma once

#include <cassert>
#include <cmath> // std::log10
#include <functional> // std::hash
#include <tuple> // std::pair
#include <memory> // allocator traits
#include <utility> // std::declval
#include "allocator.hpp"
#include "vector.hpp"
namespace ftl {

  namespace detail {
    uint32_t hash_table_primes[ ]{
      29u, 59u, 127u, 257u, 521u, 1049u,
      2099u, 4201u, 8419u, 16843u, 33703u,
      67409u, 134837u, 269683u, 539389u,
      1078787u
    };
#define FTL_DEFINE_MOD_FUNC( N ) uint64_t mod##N(uint64_t value) { return value % N##ull; }
    FTL_DEFINE_MOD_FUNC(29)
    FTL_DEFINE_MOD_FUNC(59)
    FTL_DEFINE_MOD_FUNC(127)
    FTL_DEFINE_MOD_FUNC(257)
    FTL_DEFINE_MOD_FUNC(521)
    FTL_DEFINE_MOD_FUNC(1049)
    FTL_DEFINE_MOD_FUNC(2099)
    FTL_DEFINE_MOD_FUNC(4201)
    FTL_DEFINE_MOD_FUNC(8419)
    FTL_DEFINE_MOD_FUNC(16843)
    FTL_DEFINE_MOD_FUNC(33703)
    FTL_DEFINE_MOD_FUNC(67409)
    FTL_DEFINE_MOD_FUNC(134837)
    FTL_DEFINE_MOD_FUNC(269683)
    FTL_DEFINE_MOD_FUNC(539389)
    FTL_DEFINE_MOD_FUNC(1078787)
#undef FTL_DEFINE_MOD_FUNC

    uint32_t next_table_size( uint32_t prime ) {
      for( unsigned i{ 0 }; i < sizeof( hash_table_primes ) / sizeof(*hash_table_primes); ++i ) {
        if( hash_table_primes[ i ] > prime ) return hash_table_primes[ i ];
      }
      assert( false && "Hash table has exceeded its max_size." );
    }
  }
  /*
  * design decisions:
  * Open addressing
  * linear probing
  * Max probe length before grow: log(n)
  * Robin Hood hashing
  * Default prime size table, hash can set it to pow2 sizes
  * Size is always actually prime + log(prime) to avoid bounds checks
  * k+v pairs are stored in one array
  * hashes + index into kv pair array are stored in another array
  */
  template<typename Key,
    typename Value,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    typename Allocator = ftl::default_allocator< std::pair<const Key, Value> >
  >
  class hash_map {
  public:
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<const Key, Value>;
    using size_type = typename std::allocator_traits<allocator_type>::size_type;
    using difference_type = typename std::allocator_traits<allocator_type>::difference_type;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using allocator_type = Allocator;
    using reference = value_type&;
    using const_reference = const reference;
    using pointer = typename std::allocator_traits<allocator_type>::pointer;
    using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;
    using hash_type = decltype( std::declval<hasher>( )( std::declval<key_type>( ) ) );
    using type = hash_map<key_type, value_type, key_equal, allocator_type>;
    using iterator = typename ::ftl::vector<value_type>::iterator;
    using const_iterator = typename ::ftl::vector<value_type>::const_iterator;
    using insert_return_type = std::pair<iterator, bool>;
    // constructors/destructor


    // capacity
    size_type empty( ) const noexcept { return m_values.empty( ); }
    size_type size( ) const noexcept { return m_values.size( ); }
    size_type max_size( ) const noexcept {
      //@@TODO: update this to support other allocation strategies (like power 2)
      return ::ftl::detail::hash_table_primes[ sizeof(::ftl::detail::hash_table_primes) 
                                             / sizeof(::ftl::detail::hash_table_primes[0])
                                             - 1u ];
    }
    size_type capacity( ) const noexcept { 
      return static_cast<float>( m_capacity + max_probe_count() ) * m_max_load_factor; 
    }
    // iterators
    iterator begin( ) const noexcept { return m_values.begin( ); }
    const_iterator cbegin( ) const noexcept { return m_values.cbegin( ); }
    iterator end( ) const noexcept { return m_values.end( ); }
    const_iterator cend( ) const noexcept { return m_values.cend( ); }
    //
    template<typename... Args>
    ::std::pair<iterator, bool> emplace( Args&&... args ) {
      m_values.emplace_back( ::std::forward<Args>( args )... );
      const hash_type hash{ m_hasher( m_values.back( ).first ) };
      insert_return_type collision{ hash_collision( hash ) };
      if( collision.second ) {
        m_values.pop_back( );
        return { collision.first, false };
      } else {
        insert_hash( m_hashes, { hash, m_values.size( ) - 1 } );
        return { m_values.end( )--, true };
      }
    }
    void clear( ) {
      m_hashes.clear( );
      m_values.clear( );
    }

    insert_return_type insert( value_type&& value ) {
      return emplace( ::std::forward<value_type&&>( value ) );
    }

    insert_return_type insert( const_reference value ) {
      insert_return_type result{ this->end( ), false };
      hash_type hash{ m_hasher( value.first ) };
      insert_return_type collision{ hash_collision( hash ) };
      if( collision.second ) return { collision.first, false }; // Failed: hash exists
      else {
        m_values.emplace_back( value );
        insert_hash( m_hashes, { hash, m_values.size( ) - 1 } );
      }
      return { --m_values.end( ), true };
    }

    template<typename M>
    insert_return_type insert_or_assign( const key_type &k, M&& obj ) {
      hash_type hash{ m_hasher( k ) };
      auto collision{ hash_collision( hash ) };
      if( collision == m_hashes.end( ) ) {
        return insert( std::move( value_type{ k, std::move( obj ) } ) );
      } else {
        m_values[ collision->second ].first = k;
        m_values[ collision->second ].second = std::move( obj );
        return { { m_values.begin( ) + collision->second }, true };
      }
    }

    void reserve( size_type n ) {
      if( this->capacity() > n  ) return;
      m_max_probe_count = static_cast< size_type >( std::log2( n ) );
      size_type new_size{ ::ftl::detail::next_table_size( n ) };
      size_type new_capacity{ new_size + max_probe_count( ) };
      ftl::vector< std::pair< hash_type, size_type > > hashes( new_capacity, tombstone );
      for(auto &it : m_hashes ){ 
        insert_hash( hashes, it );
      }
      m_hashes = std::move( hashes );
      m_values.reserve( new_capacity );
      m_capacity = new_size;
    }

    iterator find( const key_type &key ) {
      hash_type hash{ m_hasher( key ) };
      size_type index{ hash % m_capacity };
      for( size_type i{ 0 }; i <= max_probe_count( ); ++i ) {
        hash_type &value{ m_hashes[ index + i ] };
        iterator element{ m_values.begin( ) + value.second };
        if( value.first == hash && m_key_equal( key, element->first ) ) {
          return element;
        }
      }
      return m_values.end( );
    }

    const_iterator find( const key_type &key ) const {
      hash_type hash{ m_hasher( key ) };
      size_type index{ hash % m_capacity };
      for( size_type i{ 0 }; i <= max_probe_count( ); ++i ) {
        hash_type &value{ m_hashes[ index + i ] };
        const_iterator element{ m_values.cbegin( ) + value.second };
        if( value.first == hash && m_key_equal( key, element->first ) ) {
          return element;
        }
      }
      return m_values.cend( );
    }

    iterator find( key_type &&key ) {
      hash_type hash{ m_hasher( key ) };
      size_type index{ hash % m_capacity };
      for( size_type i{ 0 }; i <= max_probe_count( ); ++i ) {
        hash_type &value{ m_hashes[ index + i ] };
        iterator element{ m_values.begin( ) + value.second };
        if( value.first == hash && m_key_equal( key, element->first ) ) {
          return element;
        }
      }
      return m_values.end( );
    }

    const_iterator find( key_type &&key ) const {
      hash_type hash{ m_hasher( key ) };
      size_type index{ hash % m_capacity };
      for( size_type i{ 0 }; i <= max_probe_count( ); ++i ) {
        hash_type &value{ m_hashes[ index + i ] };
        const_iterator element{ m_values.cbegin( ) + value.second };
        if( value.first == hash && m_key_equal( key, element->first ) ) {
          return element;
        }
      }
      return m_values.cend( );
    }

    // find() version which doesn't perform validation of keys.
    // By omitting the key compare, speed is improved substantially.
    // The cost of this is two unique keys with a hash collision may 
    //   return the wrong value.
    iterator find_fast( const key_type &key ) {
      hash_type hash{ m_hasher( key ) };
      size_type index{ hash % m_capacity };
      for( size_type i{ 0 }; i <= max_probe_count( ); ++i ) {
        hash_type &value{ m_hashes[ index + i ] };
        iterator element{ m_values.begin( ) + value.second };
        if( value.first == hash ) {
          return element;
        }
      }
      return m_values.end( );
    }

    // find() version which doesn't perform validation of keys.
    // By omitting the key compare, speed is improved substantially.
    // The cost of this is two unique keys with a hash collision may 
    //   return the wrong value.
    const_iterator find_fast( const key_type &key ) const {
      hash_type hash{ m_hasher( key ) };
      size_type index{ hash % m_capacity };
      for( size_type i{ 0 }; i <= max_probe_count( ); ++i ) {
        hash_type &value{ m_hashes[ index + i ] };
        const_iterator element{ m_values.cbegin( ) + value.second };
        if( value.first == hash ) {
          return element;
        }
      }
      return m_values.cend( );
    }

    // find() version which doesn't perform validation of keys.
    // By omitting the key compare, speed is improved substantially.
    // The cost of this is two unique keys with a hash collision may 
    //   return the wrong value.
    iterator find_fast( key_type &&key ) {
      hash_type hash{ m_hasher( key ) };
      size_type index{ hash % m_capacity };
      for( size_type i{ 0 }; i <= max_probe_count( ); ++i ) {
        hash_type &value{ m_hashes[ index + i ] };
        iterator element{ m_values.begin( ) + value.second };
        if( value.first == hash ) {
          return element;
        }
      }
      return m_values.end( );
    }

    // find() version which doesn't perform validation of keys.
    // By omitting the key compare, speed is improved substantially.
    // The cost of this is two unique keys with a hash collision may 
    //   return the wrong value.
    const_iterator find_fast( key_type &&key ) const {
      hash_type hash{ m_hasher( key ) };
      size_type index{ hash % m_capacity };
      for( size_type i{ 0 }; i <= max_probe_count( ); ++i ) {
        hash_type &value{ m_hashes[ index + i ] };
        const_iterator element{ m_values.cbegin( ) + value.second };
        if( value.first == hash ) {
          return element;
        }
      }
      return m_values.cend( );
    }

  private:
    using probe_table_iterator = ftl::vector<::std::pair<hash_type, size_type>>::iterator;
    static const std::pair< hash_type, size_type > tombstone{ 0u, 0u };

    ftl::vector< std::pair< hash_type, size_type >, allocator_type > m_hashes;
    ftl::vector< value_type, allocator_type > m_values;
    hasher m_hasher;
    key_equal m_key_equal;
    float m_max_load_factor{ 0.5f };
    size_type m_capacity{ 0u };
    size_type m_max_probe_count{ 0u };

    // safety + assumption checks
    static_assert( reinterpret_cast< type * >( nullptr ) == reinterpret_cast< type * >( nullptr )->m_hashes, "m_hashes not at the start of class." );


    size_type max_probe_count( ) const noexcept { return m_max_probe_count; }

    probe_table_iterator hash_collision( hash_type hash ) {
      size_type index{ hash % m_hashes.size( ) };
      for( size_type i{ 0 }; i < m_max_probe_count; ++i ) {
        if( m_hashes[ index + i ].first == hash ) return { m_hashes.begin() + (index + i) };
      }
      return { m_hashes.end( ) };
    }

    void insert_hash( ftl::vector< std::pair<hash_type, size_type> > &vec, std::pair<hash_type, size_type> hash ) {
      size_type index{ it.first % new_size };
      unsigned probe_count{ 0 };
      while( probe_count <= m_max_probe_count && vec[ index + probe_count ] != tombstone ) {
        unsigned distance_from_optimal{ vec[ index + probe_count ].first % new_size };
        if( distance_from_optimal < probe_count ) {
          std::swap( vec[ index + probe_count ], hash );
          probe_count = distance_from_optimal;
        } 
        ++probe_count;
      }
      if( vec[ index + probe_count ] == tombstone ) {
        vec[ index + probe_count ] = hash;
      } else if (probe_count > m_max_probe_count ) {
        assert( false && "An internal error has occurred. Please file a report with the FTL project on github.com/masstronaut/FTL " );
      }
    }
  };


}






