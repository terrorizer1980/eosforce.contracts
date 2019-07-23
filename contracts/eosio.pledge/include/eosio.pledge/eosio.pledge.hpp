/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   class [[eosio::contract("eosio.pledge")]] pledge : public contract {
      public:
         using contract::contract;

      private:
         struct [[eosio::table]] pledge_type {
            name        pledge_name;
            account_name   deduction_account;

            uint64_t primary_key()const { return pledge_name.value; }
         };

         struct [[eosio::table]] pledge_info {
            name     pledge_name;
            asset    pledge;
            asset    deduction;

            uint64_t primary_key()const { return pledge_name.value; }
         };

         struct [[eosio::table]] reward_info {
            asset reward;

            uint64_t primary_key()const { return reward.symbol.code().raw(); }
         };

         typedef eosio::multi_index< "pledgetype"_n, pledge_type > pledgetypes;
         typedef eosio::multi_index< "pledge"_n, pledge_info > pledges;
         typedef eosio::multi_index< "reward"_n, reward_info > rewards;
      public:
         // [[eosio::action]] void addtype( const name& pledge_name,
         //                               const account_name& deduction_account
         //                               const string& memo );

         // [[eosio::action]] void addpledge( const name& pledge_name,
         //                               const asset& quantity,
         //                               const string& memo );

         // [[eosio::action]] void deduction( const name& pledge_name,
         //                               const account_name& debitee
         //                               const asset& quantity,
         //                               const string& memo );
         // [[eosio::action]] void withdraw( const name& pledge_name,
         //                               const account_name& pledger,
         //                               const asset& quantity,
         //                               const string& memo );
         // [[eosio::action]] void getreward( const account_name& pledger,
         //                               const asset& quantity,
         //                               const string& memo );
   };

} /// namespace eosio
