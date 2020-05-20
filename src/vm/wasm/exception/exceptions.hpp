#pragma once

#include"wasm/exception/exception.hpp"
//#include"boost/core/typeinfo.hpp"

using namespace wasm_chain;

namespace wasm_chain {

#define CHAIN_ASSERT( expr, exception_type, ... )       \
	if ( !( expr ) ) {                            \
		throw exception_type( CHAIN_LOG_MESSAGE( log_level::warn, __VA_ARGS__ ) );}

#define CHAIN_THROW( exception_type, ... )       \
		throw exception_type( CHAIN_LOG_MESSAGE( log_level::warn, __VA_ARGS__ ) );

#define CHAIN_RETHROW_EXCEPTIONS(exception_type, ... ) \
   catch( const std::bad_alloc& ) {\
      throw;\
   } catch (wasm_chain::chain_exception& e) { \
      CHAIN_RETHROW_EXECPTION(e,log_level::warn, __VA_ARGS__ )\
   } catch (wasm_chain::exception& e) { \
      exception_type new_exception(CHAIN_LOG_MESSAGE( log_level::warn, __VA_ARGS__ )); \
      for (const auto& log: e.get_log()) { \
         new_exception.append_log(log); \
      } \
      throw new_exception; \
   } catch( const std::exception& e ) {  \
      exception_type wasme(CHAIN_LOG_MESSAGE( log_level::warn, tfm::format(__VA_ARGS__) + " " + e.what())); \
      throw wasme;\
   } catch( ... ) {  \
      throw wasm_chain::unhandled_exception( \
                CHAIN_LOG_MESSAGE( log_level::warn, __VA_ARGS__), \
                std::current_exception() ); \
   }

#define CHAIN_CAPTURE_AND_RETHROW( ... ) \
   catch( wasm_chain::exception& er ) { \
      CHAIN_RETHROW_EXECPTION( er, log_level::warn, tfm::format(__VA_ARGS__) ); \
   } catch( const std::exception& e ) {  \
      wasm_chain::exception wasme( \
                CHAIN_LOG_MESSAGE( log_level::warn,  ": " + tfm::format(__VA_ARGS__) + " " + e.what()), \
                wasm_chain::std_exception_code,\
                "std_exception", \
                e.what() ); throw wasme;\
   } catch( ... ) {  \
      throw wasm_chain::unhandled_exception( \
                CHAIN_LOG_MESSAGE( log_level::warn, tfm::format(__VA_ARGS__)), \
                std::current_exception() ); \
   }


  #define CHAIN_EXCEPTION_APPEND_LOG( E, LOG_LEVEL, ... ) \
      E.append_log( CHAIN_LOG_MESSAGE( LOG_LEVEL, __VA_ARGS__) );

  #define CHAIN_RETHROW_EXECPTION(E, LOG_LEVEL, ...) \
      CHAIN_EXCEPTION_APPEND_LOG(E, LOG_LEVEL, __VA_ARGS__ ) \
      throw;


   CHAIN_DECLARE_DERIVED_EXCEPTION( chain_exception, wasm_chain::exception,
                                                 3000000, "blockchain exception" )
   /**
    *  chain_exception
    *   |- chain_type_exception
    *   |- fork_database_exception
    *   |- block_validate_exception
    *   |- transaction_exception
    *   |- inline_transaction_validate_exception
    *   |- database_exception
    *   |- wasm_exception
    *   |- resource_exhausted_exception
    *   |- misc_exception
    *   |- wallet_exception
    *   |- whitelist_blacklist_exception
    *   |- controller_emit_signal_exception
    *   |- abi_exception
    *   |- contract_exception
    *   |- producer_exception
    *   |- reversible_blocks_exception
    *   |- block_log_exception
    *   |- resource_limit_exception
    *   |- level_db_exception
    *   |- contract_api_exception
    */


    CHAIN_DECLARE_DERIVED_EXCEPTION( chain_type_exception, chain_exception,
                                  3010000, "chain type exception" )

      CHAIN_DECLARE_DERIVED_EXCEPTION( regid_type_exception,               chain_type_exception,
                                    3010001, "Invalid regid" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( public_key_type_exception,         chain_type_exception,
                                    3010002, "Invalid public key" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( private_key_type_exception,        chain_type_exception,
                                    3010003, "Invalid private key" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( authority_type_exception,          chain_type_exception,
                                    3010004, "Invalid authority" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( action_type_exception,             chain_type_exception,
                                    3010005, "Invalid action" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( transaction_type_exception,        chain_type_exception,
                                    3010006, "Invalid transaction" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( abi_type_exception,                chain_type_exception,
                                    3010007, "Invalid ABI" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( block_id_type_exception,           chain_type_exception,
                                    3010008, "Invalid block ID" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( transaction_id_type_exception,     chain_type_exception,
                                    3010009, "Invalid transaction ID" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( packed_transaction_type_exception, chain_type_exception,
                                    3010010, "Invalid packed transaction" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( asset_type_exception,              chain_type_exception,
                                    3010011, "Invalid asset" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( chain_id_type_exception,           chain_type_exception,
                                    3010012, "Invalid chain ID" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( fixed_key_type_exception,          chain_type_exception,
                                    3010013, "Invalid fixed key" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( symbol_type_exception,             chain_type_exception,
                                    3010014, "Invalid symbol" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( unactivated_key_type,              chain_type_exception,
                                    3010015, "Key type is not a currently activated type" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( unactivated_signature_type,        chain_type_exception,
                                    3010016, "Signature type is not a currently activated type" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( fork_database_exception, chain_exception,
                                 3020000, "Fork database exception" )

      CHAIN_DECLARE_DERIVED_EXCEPTION( fork_db_block_not_found, fork_database_exception,
                                    3020001, "Block can not be found" )


   CHAIN_DECLARE_DERIVED_EXCEPTION( block_validate_exception, chain_exception,
                                 3030000, "Block exception" )

      CHAIN_DECLARE_DERIVED_EXCEPTION( unlinkable_block_exception, block_validate_exception,
                                    3030001, "Unlinkable block" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( block_tx_output_exception,   block_validate_exception,
                                    3030002, "Transaction outputs in block do not match transaction outputs from applying block" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( block_concurrency_exception, block_validate_exception,
                                    3030003, "Block does not guarantee concurrent execution without conflicts" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( block_lock_exception,        block_validate_exception,
                                    3030004, "Shard locks in block are incorrect or mal-formed" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( block_resource_exhausted,    block_validate_exception,
                                    3030005, "Block exhausted allowed resources" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( block_too_old_exception,     block_validate_exception,
                                    3030006, "Block is too old to push" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( block_from_the_future,       block_validate_exception,
                                    3030007, "Block is from the future" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( wrong_signing_key,           block_validate_exception,
                                    3030008, "Block is not signed with expected key" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( wrong_producer,              block_validate_exception,
                                    3030009, "Block is not signed by expected producer" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_block_header_extension, block_validate_exception,
                                    3030010, "Invalid block header extension" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( ill_formed_protocol_feature_activation, block_validate_exception,
                                    3030011, "Block includes an ill-formed protocol feature activation extension" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_block_extension, block_validate_exception,
                                    3030012, "Invalid block extension" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( ill_formed_additional_block_signatures_extension, block_validate_exception,
                                    3030013, "Block includes an ill-formed additional block signature extension" )


   CHAIN_DECLARE_DERIVED_EXCEPTION( transaction_exception,             chain_exception,
                                 3040000, "Transaction exception" )

      CHAIN_DECLARE_DERIVED_EXCEPTION( tx_decompression_error,      transaction_exception,
                                    3040001, "Error decompressing transaction" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( tx_no_action,                transaction_exception,
                                    3040002, "Transaction should have at least one normal action" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( tx_no_auths,                 transaction_exception,
                                    3040003, "Transaction should have at least one required authority" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( cfa_irrelevant_auth,         transaction_exception,
                                    3040004, "Context-free action should have no required authority" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( expired_tx_exception,        transaction_exception,
                                    3040005, "Expired Transaction" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( tx_exp_too_far_exception,    transaction_exception,
                                    3040006, "Transaction Expiration Too Far" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_ref_block_exception, transaction_exception,
                                    3040007, "Invalid Reference Block" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( tx_duplicate,                transaction_exception,
                                    3040008, "Duplicate transaction" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( deferred_tx_duplicate,       transaction_exception,
                                    3040009, "Duplicate deferred transaction" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( cfa_inside_generated_tx,     transaction_exception,
                                    3040010, "Context free action is not allowed inside generated transaction" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( tx_not_found,     transaction_exception,
                                    3040011, "The transaction can not be found" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( too_many_tx_at_once,          transaction_exception,
                                    3040012, "Pushing too many transactions at once" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( tx_too_big,                   transaction_exception,
                                    3040013, "Transaction is too big" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( unknown_transaction_compression, transaction_exception,
                                    3040014, "Unknown transaction compression" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_transaction_extension, transaction_exception,
                                    3040015, "Invalid transaction extension" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( ill_formed_deferred_transaction_generation_context, transaction_exception,
                                    3040016, "Transaction includes an ill-formed deferred transaction generation context extension" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( disallowed_transaction_extensions_bad_block_exception, transaction_exception,
                                    3040017, "Transaction includes disallowed extensions (invalid block)" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( tx_resource_exhaustion, transaction_exception,
                                    3040018, "Transaction exceeded transient resource limit" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( transaction_execute_exception, transaction_exception,
                                    3040019, "Transaction execute error" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( transaction_trace_access_exception, transaction_exception,
                                    3040020, "Transaction trace access exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( fee_exhausted_exception, transaction_exception,
                                    3040021, "Transaction trace access exception" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( inline_transaction_validate_exception, chain_exception,
                                 3050000, "Action validate exception" )

      CHAIN_DECLARE_DERIVED_EXCEPTION( account_name_exists_exception, inline_transaction_validate_exception,
                                    3050001, "Account name already exists" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_action_args_exception, inline_transaction_validate_exception,
                                    3050002, "Invalid Action Arguments" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( wasm_assert_message_exception, inline_transaction_validate_exception,
                                    3050003, "wasm_assert_message assertion failure" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( wasm_assert_code_exception, inline_transaction_validate_exception,
                                    3050004, "wasm_assert_code assertion failure" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( action_not_found_exception, inline_transaction_validate_exception,
                                    3050005, "Action can not be found" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( action_data_and_struct_mismatch, inline_transaction_validate_exception,
                                    3050006, "Mismatch between action data and its struct" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( unaccessible_api, inline_transaction_validate_exception,
                                    3050007, "Attempt to use unaccessible API" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( abort_called, inline_transaction_validate_exception,
                                    3050008, "Abort Called" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( inline_action_too_big, inline_transaction_validate_exception,
                                    3050009, "Inline Action exceeds maximum size limit" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( unauthorized_ram_usage_increase, inline_transaction_validate_exception,
                                    3050010, "Action attempts to increase RAM usage of account without authorization" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( restricted_error_code_exception, inline_transaction_validate_exception,
                                    3050011, "wasm_assert_code assertion failure uses restricted error code value" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( wasm_assert_exception, inline_transaction_validate_exception,
                                    3050004, "wasm_assert assertion failure" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( database_exception, chain_exception,
                                 3060000, "Database exception" )

      CHAIN_DECLARE_DERIVED_EXCEPTION( permission_query_exception,     database_exception,
                                    3060001, "Permission Query Exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( account_query_exception,        database_exception,
                                    3060002, "Account Query Exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( contract_table_query_exception, database_exception,
                                    3060003, "Contract Table Query Exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( contract_query_exception,       database_exception,
                                    3060004, "Contract Query Exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( bad_database_version_exception, database_exception,
                                    3060005, "Database is an unknown or unsupported version" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( guard_exception, database_exception,
                                 3060100, "Guard Exception" )

      CHAIN_DECLARE_DERIVED_EXCEPTION( database_guard_exception, guard_exception,
                                    3060101, "Database usage is at unsafe levels" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( reversible_guard_exception, guard_exception,
                                    3060102, "Reversible block log usage is at unsafe levels" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( wasm_exception, chain_exception,
                                 3070000, "WASM Exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( page_memory_error,        wasm_exception,
                                    3070001, "Error in WASM page memory" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( wasm_execution_error,     wasm_exception,
                                    3070002, "Runtime Error Processing WASM" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( wasm_serialization_error, wasm_exception,
                                    3070003, "Serialization Error Processing WASM" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( overlapping_memory_error, wasm_exception,
                                    3070004, "memcpy with overlapping memory" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( eosvm_exception, wasm_exception,
                                    3070005, "eosvm exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( eosvm_jit_exception, wasm_exception,
                                    3070006, "eosvm jit exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( vm_type_mismatch, wasm_exception,
                                    3070006, "vm type mismatch exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( code_parse_exception, wasm_exception,
                                    3070007, "code parse exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( wasm_memory_exception, wasm_exception,
                                    3070008, "wasm memory exception" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( resource_exhausted_exception, chain_exception,
                                 3080000, "Resource exhausted exception" )

      CHAIN_DECLARE_DERIVED_EXCEPTION( ram_usage_exceeded, resource_exhausted_exception,
                                    3080001, "Account using more than allotted RAM usage" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( tx_net_usage_exceeded, resource_exhausted_exception,
                                    3080002, "Transaction exceeded the current network usage limit imposed on the transaction" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( block_net_usage_exceeded, resource_exhausted_exception,
                                    3080003, "Transaction network usage is too much for the remaining allowable usage of the current block" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( tx_cpu_usage_exceeded, resource_exhausted_exception,
                                    3080004, "Transaction exceeded the current CPU usage limit imposed on the transaction" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( block_cpu_usage_exceeded, resource_exhausted_exception,
                                    3080005, "Transaction CPU usage is too much for the remaining allowable usage of the current block" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( deadline_exception, resource_exhausted_exception,
                                    3080006, "Transaction took too long" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( greylist_net_usage_exceeded, resource_exhausted_exception,
                                    3080007, "Transaction exceeded the current greylisted account network usage limit" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( wasm_timeout_exception, resource_exhausted_exception,
                                    3080008, "wasm time out exception" )


   CHAIN_DECLARE_DERIVED_EXCEPTION( authorization_exception, chain_exception,
                                 3090000, "Authorization exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( tx_duplicate_sig,             authorization_exception,
                                    3090001, "Duplicate signature included" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( tx_irrelevant_sig,            authorization_exception,
                                    3090002, "Irrelevant signature included" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( unsatisfied_authorization,    authorization_exception,
                                    3090003, "Provided keys, permissions, and delays do not satisfy declared authorizations" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( missing_auth_exception,       authorization_exception,
                                    3090004, "Missing required authority" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( irrelevant_auth_exception,    authorization_exception,
                                    3090005, "Irrelevant authority included" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( insufficient_delay_exception, authorization_exception,
                                    3090006, "Insufficient delay" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_permission,           authorization_exception,
                                    3090007, "Invalid Permission" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( unlinkable_min_permission_action, authorization_exception,
                                    3090008, "The action is not allowed to be linked with minimum permission" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( misc_exception, chain_exception,
                                 3100000, "Miscellaneous exception" )

      CHAIN_DECLARE_DERIVED_EXCEPTION( rate_limiting_state_inconsistent,       misc_exception,
                                    3100001, "Internal state is no longer consistent" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( unknown_block_exception,                misc_exception,
                                    3100002, "Unknown block" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( unknown_transaction_exception,          misc_exception,
                                    3100003, "Unknown transaction" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( fixed_reversible_db_exception,          misc_exception,
                                    3100004, "Corrupted reversible block database was fixed" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( extract_genesis_state_exception,        misc_exception,
                                    3100005, "Extracted genesis state from blocks.log" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( subjective_block_production_exception,  misc_exception,
                                    3100006, "Subjective exception thrown during block production" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( multiple_voter_info,                    misc_exception,
                                    3100007, "Multiple voter info detected" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( unsupported_feature,                    misc_exception,
                                    3100008, "Feature is currently unsupported" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( node_management_success,                misc_exception,
                                    3100009, "Node management operation successfully executed" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( json_parse_exception,                misc_exception,
                                    3100010, "JSON parse exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( sig_variable_size_limit_exception,      misc_exception,
                                    3100011, "Variable length component of signature too large" )


   CHAIN_DECLARE_DERIVED_EXCEPTION( wallet_exception, chain_exception,
                                 3120000, "Wallet exception" )

      CHAIN_DECLARE_DERIVED_EXCEPTION( wallet_exist_exception,            wallet_exception,
                                    3120001, "Wallet already exists" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( wallet_nonexistent_exception,      wallet_exception,
                                    3120002, "Nonexistent wallet" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( wallet_locked_exception,           wallet_exception,
                                    3120003, "Locked wallet" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( wallet_missing_pub_key_exception,  wallet_exception,
                                    3120004, "Missing public key" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( wallet_invalid_password_exception, wallet_exception,
                                    3120005, "Invalid wallet password" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( wallet_not_available_exception,    wallet_exception,
                                    3120006, "No available wallet" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( wallet_unlocked_exception,         wallet_exception,
                                    3120007, "Already unlocked" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( key_exist_exception,               wallet_exception,
                                    3120008, "Key already exists" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( key_nonexistent_exception,         wallet_exception,
                                    3120009, "Nonexistent key" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( unsupported_key_type_exception,    wallet_exception,
                                    3120010, "Unsupported key type" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_lock_timeout_exception,    wallet_exception,
                                    3120011, "Wallet lock timeout is invalid" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( secure_enclave_exception,          wallet_exception,
                                    3120012, "Secure Enclave Exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( wallet_sign_exception,          wallet_exception,
                                    3120013, "Wallet Sign exception" )


   CHAIN_DECLARE_DERIVED_EXCEPTION( whitelist_blacklist_exception,   chain_exception,
                                 3130000, "Actor or contract whitelist/blacklist exception" )

      CHAIN_DECLARE_DERIVED_EXCEPTION( actor_whitelist_exception,    whitelist_blacklist_exception,
                                    3130001, "Authorizing actor of transaction is not on the whitelist" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( actor_blacklist_exception,    whitelist_blacklist_exception,
                                    3130002, "Authorizing actor of transaction is on the blacklist" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( contract_whitelist_exception, whitelist_blacklist_exception,
                                    3130003, "Contract to execute is not on the whitelist" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( contract_blacklist_exception, whitelist_blacklist_exception,
                                    3130004, "Contract to execute is on the blacklist" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( action_blacklist_exception,   whitelist_blacklist_exception,
                                    3130005, "Action to execute is on the blacklist" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( key_blacklist_exception,      whitelist_blacklist_exception,
                                    3130006, "Public key in authority is on the blacklist" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( controller_emit_signal_exception, chain_exception,
                                 3140000, "Exceptions that are allowed to bubble out of emit calls in controller" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( checkpoint_exception,          controller_emit_signal_exception,
                                   3140001, "Block does not match checkpoint" )


   CHAIN_DECLARE_DERIVED_EXCEPTION( abi_exception,                           chain_exception,
                                 3015000, "ABI exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( abi_not_found_exception,              abi_exception,
                                    3015001, "No ABI found" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_ricardian_clause_exception,   abi_exception,
                                    3015002, "Invalid Ricardian Clause" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_ricardian_action_exception,   abi_exception,
                                    3015003, "Invalid Ricardian Action" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_type_inside_abi,           abi_exception,
                                    3015004, "The type defined in the ABI is invalid" ) // Not to be confused with abi_type_exception
      CHAIN_DECLARE_DERIVED_EXCEPTION( duplicate_abi_type_def_exception,     abi_exception,
                                    3015005, "Duplicate type definition in the ABI" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( duplicate_abi_struct_def_exception,   abi_exception,
                                    3015006, "Duplicate struct definition in the ABI" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( duplicate_abi_action_def_exception,   abi_exception,
                                    3015007, "Duplicate action definition in the ABI" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( duplicate_abi_table_def_exception,    abi_exception,
                                    3015008, "Duplicate table definition in the ABI" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( duplicate_abi_err_msg_def_exception,  abi_exception,
                                    3015009, "Duplicate error message definition in the ABI" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( abi_serialization_deadline_exception, abi_exception,
                                    3015010, "ABI serialization time has exceeded the deadline" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( abi_recursion_depth_exception,        abi_exception,
                                    3015011, "ABI recursive definition has exceeded the max recursion depth" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( abi_circular_def_exception,           abi_exception,
                                    3015012, "Circular definition is detected in the ABI" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( unpack_exception,                     abi_exception,
                                    3015013, "Unpack data exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( pack_exception,                     abi_exception,
                                    3015014, "Pack data exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( duplicate_abi_variant_def_exception,  abi_exception,
                                    3015015, "Duplicate variant definition in the ABI" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( unsupported_abi_version_exception,  abi_exception,
                                    3015016, "ABI has an unsupported version" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( abi_parse_exception,  abi_exception,
                                    3015017, "ABI parse exception" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( contract_exception,           chain_exception,
                                 3160000, "Contract exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_table_payer,             contract_exception,
                                    3160001, "The payer of the table data is invalid" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( table_access_violation,          contract_exception,
                                    3160002, "Table access violation" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_table_iterator,          contract_exception,
                                    3160003, "Invalid table iterator" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( table_not_in_cache,          contract_exception,
                                    3160004, "Table can not be found inside the cache" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( table_operation_not_permitted,          contract_exception,
                                    3160005, "The table operation is not allowed" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_contract_vm_type,          contract_exception,
                                    3160006, "Invalid contract vm type" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_contract_vm_version,          contract_exception,
                                    3160007, "Invalid contract vm version" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( set_exact_code,          contract_exception,
                                    3160008, "Contract is already running this version of code" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( wasm_file_not_found,          contract_exception,
                                    3160009, "No wasm file found" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( abi_file_not_found,           contract_exception,
                                    3160010, "No abi file found" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( table_not_found,              contract_exception,
                                    3160011, "Table not found" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( producer_exception,           chain_exception,
                                 3170000, "Producer exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( producer_priv_key_not_found,   producer_exception,
                                    3170001, "Producer private key is not available" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( missing_pending_block_state,   producer_exception,
                                    3170002, "Pending block state is missing" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( producer_double_confirm,       producer_exception,
                                    3170003, "Producer is double confirming known range" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( producer_schedule_exception,   producer_exception,
                                    3170004, "Producer schedule exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( producer_not_in_schedule,      producer_exception,
                                    3170006, "The producer is not part of current schedule" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( snapshot_directory_not_found_exception,  producer_exception,
                                    3170007, "The configured snapshot directory does not exist" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( snapshot_exists_exception,  producer_exception,
                                    3170008, "The requested snapshot already exists" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( snapshot_finalization_exception,   producer_exception,
                                    3170009, "Snapshot Finalization Exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_protocol_features_to_activate,  producer_exception,
                                    3170010, "The protocol features to be activated were not valid" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( no_block_signatures,  producer_exception,
                                    3170011, "The signer returned no valid block signatures" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( unsupported_multiple_block_signatures,  producer_exception,
                                    3170012, "The signer returned multiple signatures but that is not supported" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( reversible_blocks_exception,   chain_exception,
                                 3180000, "Reversible Blocks exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_reversible_blocks_dir,             reversible_blocks_exception,
                                    3180001, "Invalid reversible blocks directory" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( reversible_blocks_backup_dir_exist,        reversible_blocks_exception,
                                    3180002, "Backup directory for reversible blocks already existg" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( gap_in_reversible_blocks_db,          reversible_blocks_exception,
                                    3180003, "Gap in the reversible blocks database" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( block_log_exception, chain_exception,
                                 3190000, "Block log exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( block_log_unsupported_version, block_log_exception,
                                    3190001, "unsupported version of block log" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( block_log_append_fail, block_log_exception,
                                    3190002, "fail to append block to the block log" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( block_log_not_found, block_log_exception,
                                    3190003, "block log can not be found" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( block_log_backup_dir_exist, block_log_exception,
                                    3190004, "block log backup dir already exists" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( block_index_not_found, block_log_exception,
                                    3190005, "block index can not be found"  )

   CHAIN_DECLARE_DERIVED_EXCEPTION( http_exception, chain_exception,
                                 3200000, "http exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_http_client_root_cert,    http_exception,
                                    3200001, "invalid http client root certificate" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_http_response, http_exception,
                                    3200002, "invalid http response" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( resolved_to_multiple_ports, http_exception,
                                    3200003, "service resolved to multiple ports" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( fail_to_resolve_host, http_exception,
                                    3200004, "fail to resolve host" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( http_request_fail, http_exception,
                                    3200005, "http request fail" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( invalid_http_request, http_exception,
                                    3200006, "invalid http request" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( resource_limit_exception, chain_exception,
                                 3210000, "Resource limit exception" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( level_db_exception, chain_exception,
                                 3220000, "Level DB exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( level_db_insert_fail, level_db_exception,
                                 3220001, "Fail to insert new data to Level DB" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( level_db_update_fail, level_db_exception,
                                 3220002, "Fail to update existing data in Level DB" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( contract_api_exception,    chain_exception,
                                 3230000, "Contract API exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( crypto_api_exception,   contract_api_exception,
                                    3230001, "Crypto API Exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( db_api_exception,       contract_api_exception,
                                    3230002, "Database API Exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( arithmetic_exception,   contract_api_exception,
                                    3230003, "Arithmetic Exception" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( snapshot_exception,    chain_exception,
                                 3240000, "Snapshot exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( snapshot_validation_exception,   snapshot_exception,
                                    3240001, "Snapshot Validation Exception" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( protocol_feature_exception,    chain_exception,
                                 3250000, "Protocol feature exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( protocol_feature_validation_exception, protocol_feature_exception,
                                    3250001, "Protocol feature validation exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( protocol_feature_bad_block_exception, protocol_feature_exception,
                                    3250002, "Protocol feature exception (invalid block)" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( protocol_feature_iterator_exception, protocol_feature_exception,
                                    3250003, "Protocol feature iterator exception" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( native_contract_exception,    chain_exception,
                                 3260000, "Native contract exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( native_contract_assert_exception, native_contract_exception,
                                    3260001, "Native contract assert exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( native_contract_access_exception, native_contract_exception,
                                    3260002, "Native contract access exception" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( account_exception,    chain_exception,
                                 3270000, "Account exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( account_access_exception, account_exception,
                                    3270001, "Account access exception" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( size_exceeds_exception,    chain_exception,
                                 3280000, "Size exceeds exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( inline_transaction_data_size_exceeds_exception, size_exceeds_exception,
                                    3280001, "Inline transaction data size exceeds exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( contract_code_size_exceeds_exception, size_exceeds_exception,
                                    3280002, "contract code size exceeds exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( rpc_params_size_exceeds_exception, size_exceeds_exception,
                                    3280003, "rpc params size exceeds exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( inline_transaction_size_exceeds_exception, size_exceeds_exception,
                                    3280004, "Inline transaction size exceeds exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( recipients_size_exceeds_exception, size_exceeds_exception,
                                    3280005, "Recipients size exceeds exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( wasm_api_data_size_exceeds_exception, size_exceeds_exception,
                                    3280006, "Wasm api data size exceeds exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( array_size_exceeds_exception, size_exceeds_exception,
                                    3280007, "array size exceeds exceeds exception" )

   CHAIN_DECLARE_DERIVED_EXCEPTION( file_exception,    chain_exception,
                                 3290000, "File exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( file_read_exception, file_exception,
                                    3290001, "File read exception" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( file_write_exception, file_exception,
                                    3290002, "File write exception" )

////////////////////////////////////////////////////////////////////////////////
// native bank

      CHAIN_DECLARE_DERIVED_EXCEPTION( asset_name_exception,      chain_type_exception,
                                    3300001, "Invalid asset name" )
      CHAIN_DECLARE_DERIVED_EXCEPTION( asset_total_supply_exception,      chain_type_exception,
                                    3300001, "Invalid asset total supply" )

} //chain
