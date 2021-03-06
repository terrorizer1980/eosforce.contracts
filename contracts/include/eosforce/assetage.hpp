#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>

#include <eosforce/eosforce.hpp>

namespace eosforce {
   namespace utils{
      constexpr inline int64_t precision_base( const uint8_t decimals ) {
         constexpr uint8_t res_size = 18;
         constexpr int64_t res[res_size] = 
            {  1, 10, 100, 1000, 10000, 
               100000, 
               1000000, 
               10000000,
               100000000,
               1000000000,
               10000000000,
               100000000000,
               1000000000000,
               10000000000000,
               100000000000000,
               1000000000000000,
               10000000000000000,
               100000000000000000 };

         if( decimals < res_size ) {
            return res[decimals];
         } else {
            auto p10 = res[res_size - 1];
            for( auto p = static_cast<int64_t>(decimals - res_size + 1); 
               p > 0; --p ) {
               p10 *= 10;
            }
            return p10;
         }
      }
   }

   constexpr auto CORE_SYMBOL = eosio::symbol(eosio::symbol_code("EOS"), 4);
   constexpr auto CORE_SYMBOL_PRECISION = utils::precision_base(CORE_SYMBOL.precision());

   using eosio::symbol;
   using eosio::symbol_code;
   using eosio::asset;
   using eosio::name;
   
   static constexpr eosio::name system_account{"eosio"_n};
   static constexpr eosio::name active_permission{"active"_n};
   static constexpr eosio::name block_out_pledge{"block.out"_n};
   static constexpr eosio::name pledge_account{"eosio.pledge"_n};
   static constexpr eosio::name budget_account{"eosc.budget"_n};

   // asset age : asset age is a value equal asset * age by block num, in eosforce
   //             it used in many pos.
   struct assetage {
      inline constexpr int64_t get_age( const uint32_t curr_block_num ) const {
         return ((staked.amount / utils::precision_base(staked.symbol.precision())) * ( curr_block_num - update_height )) + age;
      }

      inline void change_staked_to( const uint32_t curr_block_num, const asset& new_staked ) {
         age = get_age( curr_block_num );
         update_height = curr_block_num;
         staked = new_staked;
      }

      inline void add_staked_by( const uint32_t curr_block_num, const asset& s ) {
         age = get_age( curr_block_num );
         update_height = curr_block_num;
         staked += s;
      }

      inline void minus_staked_by( const uint32_t curr_block_num, const asset& s ) {
         age = get_age( curr_block_num );
         update_height = curr_block_num;
         staked -= s;
      }

      inline void clean_age( const uint32_t curr_block_num ) {
         age = 0;
         update_height = curr_block_num;
      }

      asset     staked        = asset{ 0, CORE_SYMBOL };
      int64_t   age           = 0;
      uint32_t  update_height = 0;

      EOSLIB_SERIALIZE( assetage, (staked)(age)(update_height) )
   };

} // namespace eosforce