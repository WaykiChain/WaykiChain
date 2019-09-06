// Copyright (c) 2012-2013 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifdef TODO
#include "commons/bloom.h"

#include "commons/base58.h"
#include "entities/key.h"
#include "main.h"
#include "commons/serialize.h"
#include "commons/uint256.h"
#include "commons/util.h"

#include <vector>

#include <boost/test/unit_test.hpp>

using namespace std;
using namespace boost::tuples;

BOOST_AUTO_TEST_SUITE(bloom_tests)

BOOST_AUTO_TEST_CASE(bloom_create_insert_serialize)
{
	CBloomFilter filter(3, 0.01, 0, BLOOM_UPDATE_ALL);

	filter.insert(ParseHex("99108ad8ed9bb6274d3980bab5a85c048f0950c8"));
	BOOST_CHECK_MESSAGE( filter.contains(ParseHex("99108ad8ed9bb6274d3980bab5a85c048f0950c8")), "BloomFilter doesn't contain just-inserted object!");
	// One bit different in first byte
	BOOST_CHECK_MESSAGE(!filter.contains(ParseHex("19108ad8ed9bb6274d3980bab5a85c048f0950c8")), "BloomFilter contains something it shouldn't!");

	filter.insert(ParseHex("b5a2c786d9ef4658287ced5914b37a1b4aa32eee"));
	BOOST_CHECK_MESSAGE(filter.contains(ParseHex("b5a2c786d9ef4658287ced5914b37a1b4aa32eee")), "BloomFilter doesn't contain just-inserted object (2)!");

	filter.insert(ParseHex("b9300670b4c5366e95b2699e8b18bc75e5f729c5"));
	BOOST_CHECK_MESSAGE(filter.contains(ParseHex("b9300670b4c5366e95b2699e8b18bc75e5f729c5")), "BloomFilter doesn't contain just-inserted object (3)!");

	CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
	filter.Serialize(stream, SER_NETWORK, PROTOCOL_VERSION);

	vector<unsigned char> vch = ParseHex("03614e9b050000000000000001");
	vector<char> expected(vch.size());

	for (unsigned int i = 0; i < vch.size(); i++)
	expected[i] = (char)vch[i];

	BOOST_CHECK_EQUAL_COLLECTIONS(stream.begin(), stream.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(bloom_create_insert_serialize_with_tweak)
{
	// Same test as bloom_create_insert_serialize, but we add a nTweak of 100
	CBloomFilter filter(3, 0.01, 2147483649, BLOOM_UPDATE_ALL);

	filter.insert(ParseHex("99108ad8ed9bb6274d3980bab5a85c048f0950c8"));
	BOOST_CHECK_MESSAGE( filter.contains(ParseHex("99108ad8ed9bb6274d3980bab5a85c048f0950c8")), "BloomFilter doesn't contain just-inserted object!");
	// One bit different in first byte
	BOOST_CHECK_MESSAGE(!filter.contains(ParseHex("19108ad8ed9bb6274d3980bab5a85c048f0950c8")), "BloomFilter contains something it shouldn't!");

	filter.insert(ParseHex("b5a2c786d9ef4658287ced5914b37a1b4aa32eee"));
	BOOST_CHECK_MESSAGE(filter.contains(ParseHex("b5a2c786d9ef4658287ced5914b37a1b4aa32eee")), "BloomFilter doesn't contain just-inserted object (2)!");

	filter.insert(ParseHex("b9300670b4c5366e95b2699e8b18bc75e5f729c5"));
	BOOST_CHECK_MESSAGE(filter.contains(ParseHex("b9300670b4c5366e95b2699e8b18bc75e5f729c5")), "BloomFilter doesn't contain just-inserted object (3)!");

	CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
	filter.Serialize(stream, SER_NETWORK, PROTOCOL_VERSION);

	vector<unsigned char> vch = ParseHex("03ce4299050000000100008001");
	vector<char> expected(vch.size());

	for (unsigned int i = 0; i < vch.size(); i++)
	expected[i] = (char)vch[i];

	BOOST_CHECK_EQUAL_COLLECTIONS(stream.begin(), stream.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(bloom_create_insert_key)
{

	if (TEST_NET == SysCfg().NetworkID() || REGTEST_NET == SysCfg().NetworkID()) {
		return ;
	}
	string strSecret = string("5Kg1gnAjaLfKiwhhPpGS3QfRg2m6awQvaj98JCZBZQ5SuS2F15C");
	CCoinSecret vchSecret;
	BOOST_CHECK(vchSecret.SetString(strSecret));

	CKey key = vchSecret.GetKey();
	CPubKey pubkey = key.GetPubKey();
	vector<unsigned char> vchPubKey(pubkey.begin(), pubkey.end());

	CBloomFilter filter(2, 0.001, 0, BLOOM_UPDATE_ALL);
	filter.insert(vchPubKey);
	uint160 hash = pubkey.GetKeyId();
	filter.insert(vector<unsigned char>(hash.begin(), hash.end()));

	CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
	filter.Serialize(stream, SER_NETWORK, PROTOCOL_VERSION);

	vector<unsigned char> vch = ParseHex("038fc16b080000000000000001");
	vector<char> expected(vch.size());

	for (unsigned int i = 0; i < vch.size(); i++)
	expected[i] = (char)vch[i];

	BOOST_CHECK_EQUAL_COLLECTIONS(stream.begin(), stream.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(merkle_block_1)
{
	// Random real block (34ef2048a4c15e5a76e55af5cdbac0f0bd008a51ca68d233d7cb39158cb21933)
	// With 9 txes
	CBlock block;
	CDataStream stream(ParseHex("010000000000000000000000000000000000000000000000000000000000000000000000435674038526F9A70BC991D1987F59B9356600A3C109F6567F9AE0142EC3B6080000000000000000000000000000000000000000000000000000000000000000F6229057FFFF3F1F630000000000000000000000000000006400000000020101210234CE4C86DF67CE5F70FCB73B96F749734ACB9F9489F0C00D05D45DF283823BB800000101210245F9CFD64E3164DA65EB8EE5A57CB4BC6BC1D5E0E3D6FB9C51E6748DF64EB2E380B0D0AE84EBA6FF0000"), SER_NETWORK, PROTOCOL_VERSION);
	stream >> block;

	CBloomFilter filter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
	// Match the last transaction
	filter.insert(uint256S("55acb79bcffe50b9000aec43b3399b76194b285a5c132b30e42fc913b386dbe4"));

	CMerkleBlock merkleBlock(block, filter);
	BOOST_CHECK(merkleBlock.header.GetHash() == block.GetHash());

	BOOST_CHECK(merkleBlock.vMatchedTxn.size() == 1);
	pair<unsigned int, uint256> pair = merkleBlock.vMatchedTxn[0];

	BOOST_CHECK(merkleBlock.vMatchedTxn[0].second == uint256S("55acb79bcffe50b9000aec43b3399b76194b285a5c132b30e42fc913b386dbe4"));
	BOOST_CHECK(merkleBlock.vMatchedTxn[0].first == 0);

	vector<uint256> vMatched;
	BOOST_CHECK(merkleBlock.txn.ExtractMatches(vMatched) == block.GetMerkleRootHash());
	BOOST_CHECK(vMatched.size() == merkleBlock.vMatchedTxn.size());
	for (unsigned int i = 0; i < vMatched.size(); i++)
	BOOST_CHECK(vMatched[i] == merkleBlock.vMatchedTxn[i].second);

	// Also match the 8th transaction
	filter.insert(uint256S("6ecc769150621e38c12e9af6b581b755ce4ad1e62dc8984c25124a6f7d236a1f"));
	merkleBlock = CMerkleBlock(block, filter);
	BOOST_CHECK(merkleBlock.header.GetHash() == block.GetHash());

	BOOST_CHECK(merkleBlock.vMatchedTxn.size() == 2);

	BOOST_CHECK(merkleBlock.vMatchedTxn[0] == pair);

	BOOST_CHECK(merkleBlock.vMatchedTxn[1].second == uint256S("6ecc769150621e38c12e9af6b581b755ce4ad1e62dc8984c25124a6f7d236a1f"));
	BOOST_CHECK(merkleBlock.vMatchedTxn[1].first == 1);

	BOOST_CHECK(merkleBlock.txn.ExtractMatches(vMatched) == block.GetMerkleRootHash());
	BOOST_CHECK(vMatched.size() == merkleBlock.vMatchedTxn.size());
	for (unsigned int i = 0; i < vMatched.size(); i++)
	BOOST_CHECK(vMatched[i] == merkleBlock.vMatchedTxn[i].second);
}

BOOST_AUTO_TEST_CASE(merkle_block_2)
{
	// Random real block (c6c28d39d04e8ed6b92850fab5f214490e62697890eab9f1b393a8805da397f6)
	// With 4 txes
	CBlock block;
	CDataStream stream(ParseHex("02000000435D65B098E28D46947EBD7E42836D7009FA48AE3C956D42E3837C5B15F5A12419CA1F7E0F342432E269A216198AEAA0B12EEE609CDC31C061F879F29758C10D715644119D091A51C4F695DBE45F128D23B0B8F18A2B532E721F23D7CEE90000B62EA957FFFF3F1F320200000100000000000000000000006400000046304402200C583FBFD49E6E0CD7E4BBB3F5703396BC6EE60DB10D4AEC8E3C501A7BED397002205CE29A1E6883901160CEF777EA57ADF0D3EE5987657F655AC34A4AFBBC0A9C550101020200018A95C0BB0001"), SER_NETWORK, PROTOCOL_VERSION);
	stream >> block;

	CBloomFilter filter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
	// Match the first transaction
	filter.insert(uint256S("0x0dc15897f279f861c031dc9c60ee2eb1a0ea8a1916a269e23224340f7e1fca19"));

	CMerkleBlock merkleBlock(block, filter);
	BOOST_CHECK(merkleBlock.header.GetHash() == block.GetHash());

	BOOST_CHECK(merkleBlock.vMatchedTxn.size() == 1);

	BOOST_CHECK(merkleBlock.vMatchedTxn[0].second == uint256S("0x0dc15897f279f861c031dc9c60ee2eb1a0ea8a1916a269e23224340f7e1fca19"));
	BOOST_CHECK(merkleBlock.vMatchedTxn[0].first == 0);

	vector<uint256> vMatched;
	BOOST_CHECK(merkleBlock.txn.ExtractMatches(vMatched) == block.GetMerkleRootHash());
	BOOST_CHECK(vMatched.size() == merkleBlock.vMatchedTxn.size());
	for (unsigned int i = 0; i < vMatched.size(); i++)
	BOOST_CHECK(vMatched[i] == merkleBlock.vMatchedTxn[i].second);
}

BOOST_AUTO_TEST_CASE(merkle_block_2_with_update_none)
{
	// Random real block (c6c28d39d04e8ed6b92850fab5f214490e62697890eab9f1b393a8805da397f6)
	// With 4 txes
	CBlock block;
	CDataStream stream(ParseHex("02000000589C3FCD7BB776B6C0D09D979B54A1F7C4C733E06E12E04ED37C060918DA42118A2B699FE6C536DF875FD4BED5DB6ED731841580C34425C43EFF13A811C793303D2791DD4F00714F20DDF498E052887819CECAEA2467D8749972EE227F623B00B72EA957FFFF3F1F1E000000020000000000000000000000640000004630440220229366169DD9A9F249735286E3A7A968BA679B3215168B821062E1B408FAB53702200AED72B7AB004EDDE2711351A2A7577CC7CB58A742B528DE4643E63D032DD1580101020200018A95C0BB0002"), SER_NETWORK, PROTOCOL_VERSION);
	stream >> block;

	CBloomFilter filter(10, 0.000001, 0, BLOOM_UPDATE_NONE);
	// Match the first transaction
	filter.insert(uint256S("0x3093c711a813ff3ec42544c380158431d76edbd5bed45f87df36c5e69f692b8a"));

	CMerkleBlock merkleBlock(block, filter);
	BOOST_CHECK(merkleBlock.header.GetHash() == block.GetHash());

	BOOST_CHECK(merkleBlock.vMatchedTxn.size() == 1);

	BOOST_CHECK(merkleBlock.vMatchedTxn[0].second == uint256S("0x3093c711a813ff3ec42544c380158431d76edbd5bed45f87df36c5e69f692b8a"));
	BOOST_CHECK(merkleBlock.vMatchedTxn[0].first == 0);

	vector<uint256> vMatched;
	BOOST_CHECK(merkleBlock.txn.ExtractMatches(vMatched) == block.GetMerkleRootHash());
	BOOST_CHECK(vMatched.size() == merkleBlock.vMatchedTxn.size());
	for (unsigned int i = 0; i < vMatched.size(); i++)
	BOOST_CHECK(vMatched[i] == merkleBlock.vMatchedTxn[i].second);

}

BOOST_AUTO_TEST_CASE(merkle_block_3_and_serialize)
{
	// Random real block (cd0529322ed0393491966d4adb13141775dcc5a1408f495ede95e14b41185262)
	// With one tx
	CBlock block;
	CDataStream stream(ParseHex("02000000589C3FCD7BB776B6C0D09D979B54A1F7C4C733E06E12E04ED37C060918DA42118A2B699FE6C536DF875FD4BED5DB6ED731841580C34425C43EFF13A811C793303D2791DD4F00714F20DDF498E052887819CECAEA2467D8749972EE227F623B00B72EA957FFFF3F1F1E000000020000000000000000000000640000004630440220229366169DD9A9F249735286E3A7A968BA679B3215168B821062E1B408FAB53702200AED72B7AB004EDDE2711351A2A7577CC7CB58A742B528DE4643E63D032DD1580101020200018A95C0BB0002"), SER_NETWORK, PROTOCOL_VERSION);
	stream >> block;

	CBloomFilter filter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
	// Match the only transaction
	filter.insert(uint256S("0x3093c711a813ff3ec42544c380158431d76edbd5bed45f87df36c5e69f692b8a"));

	CMerkleBlock merkleBlock(block, filter);
	BOOST_CHECK(merkleBlock.header.GetHash() == block.GetHash());

	BOOST_CHECK(merkleBlock.vMatchedTxn.size() == 1);

	BOOST_CHECK(merkleBlock.vMatchedTxn[0].second == uint256S("0x3093c711a813ff3ec42544c380158431d76edbd5bed45f87df36c5e69f692b8a"));
	BOOST_CHECK(merkleBlock.vMatchedTxn[0].first == 0);

	vector<uint256> vMatched;
	BOOST_CHECK(merkleBlock.txn.ExtractMatches(vMatched) == block.GetMerkleRootHash());
	BOOST_CHECK(vMatched.size() == merkleBlock.vMatchedTxn.size());
	for (unsigned int i = 0; i < vMatched.size(); i++)
	BOOST_CHECK(vMatched[i] == merkleBlock.vMatchedTxn[i].second);

	CDataStream merkleStream(SER_NETWORK, PROTOCOL_VERSION);
	merkleStream << merkleBlock;
	//cout << "MerkleBlock:" << HexStr(merkleStream) << endl;
	//Hex: BlockHeader + txnum + txid + bits
	vector<unsigned char> vch = ParseHex("02000000589c3fcd7bb776b6c0d09d979b54a1f7c4c733e06e12e04ed37c060918da42118a2b699fe6c536df875fd4bed5db6ed731841580c34425c43eff13a811c793303d2791dd4f00714f20ddf498e052887819cecaea2467d8749972ee227f623b00b72ea957ffff3f1f1e000000020000000000000000000000640000004630440220229366169dd9a9f249735286e3a7a968ba679b3215168b821062e1b408fab53702200aed72b7ab004edde2711351a2a7577cc7cb58a742b528de4643e63d032dd15801000000018a2b699fe6c536df875fd4bed5db6ed731841580c34425c43eff13a811c793300101");

	vector<char> expected(vch.size());

	for (unsigned int i = 0; i < vch.size(); i++)
	expected[i] = (char)vch[i];

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), merkleStream.begin(), merkleStream.end());
}

BOOST_AUTO_TEST_CASE(merkle_block_4)
{
	// Random real block (6ea6fda338a47a0c57fd3bce48a7c40d3b106c7506b9b7e44005683b89ceda7d)
	// With 7 txes
	CBlock block;
	CDataStream stream(ParseHex("010000000000000000000000000000000000000000000000000000000000000000000000435674038526F9A70BC991D1987F59B9356600A3C109F6567F9AE0142EC3B6080000000000000000000000000000000000000000000000000000000000000000F6229057FFFF3F1F630000000000000000000000000000006400000000020101210234CE4C86DF67CE5F70FCB73B96F749734ACB9F9489F0C00D05D45DF283823BB800000101210245F9CFD64E3164DA65EB8EE5A57CB4BC6BC1D5E0E3D6FB9C51E6748DF64EB2E380B0D0AE84EBA6FF0000"), SER_NETWORK, PROTOCOL_VERSION);
	stream >> block;

	CBloomFilter filter(10, 0.000001, 0, BLOOM_UPDATE_ALL);
	// Match the last transaction
	filter.insert(uint256S("55acb79bcffe50b9000aec43b3399b76194b285a5c132b30e42fc913b386dbe4"));

	CMerkleBlock merkleBlock(block, filter);
	BOOST_CHECK(merkleBlock.header.GetHash() == block.GetHash());

	BOOST_CHECK(merkleBlock.vMatchedTxn.size() == 1);
	pair<unsigned int, uint256> pair = merkleBlock.vMatchedTxn[0];

	BOOST_CHECK(merkleBlock.vMatchedTxn[0].second == uint256S("55acb79bcffe50b9000aec43b3399b76194b285a5c132b30e42fc913b386dbe4"));
	BOOST_CHECK(merkleBlock.vMatchedTxn[0].first == 0);

	vector<uint256> vMatched;
	BOOST_CHECK(merkleBlock.txn.ExtractMatches(vMatched) == block.GetMerkleRootHash());
	BOOST_CHECK(vMatched.size() == merkleBlock.vMatchedTxn.size());
	for (unsigned int i = 0; i < vMatched.size(); i++)
	BOOST_CHECK(vMatched[i] == merkleBlock.vMatchedTxn[i].second);

	// Also match the 4th transaction
	filter.insert(uint256S("6ecc769150621e38c12e9af6b581b755ce4ad1e62dc8984c25124a6f7d236a1f"));
	merkleBlock = CMerkleBlock(block, filter);
	BOOST_CHECK(merkleBlock.header.GetHash() == block.GetHash());

	BOOST_CHECK(merkleBlock.vMatchedTxn.size() == 2);

	BOOST_CHECK(merkleBlock.vMatchedTxn[1].second == uint256S("6ecc769150621e38c12e9af6b581b755ce4ad1e62dc8984c25124a6f7d236a1f"));
	BOOST_CHECK(merkleBlock.vMatchedTxn[1].first == 1);

	BOOST_CHECK(merkleBlock.vMatchedTxn[0] == pair);

	BOOST_CHECK(merkleBlock.txn.ExtractMatches(vMatched) == block.GetMerkleRootHash());
	BOOST_CHECK(vMatched.size() == merkleBlock.vMatchedTxn.size());
	for (unsigned int i = 0; i < vMatched.size(); i++)
	BOOST_CHECK(vMatched[i] == merkleBlock.vMatchedTxn[i].second);
}

BOOST_AUTO_TEST_SUITE_END()
#endif //TODO
