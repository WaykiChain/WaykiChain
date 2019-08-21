
#include "./rpc/core/rpcserver.h"
#include "./rpc/core/rpcclient.h"
#include "commons/util.h"
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace boost;
using namespace json_spirit;

std::string strSecureTxHash("");
std::string strSecureBetTxHash("");

int TestCallRPC(std::string strMethod, const std::vector<std::string> &vParams, std::string &strRet) {

	string strPrint;
	int nRet;
	Array params = RPCConvertValues(strMethod, vParams);

	Object reply = CallRPC(strMethod, params);

	// Parse reply
	const Value& result = find_value(reply, "result");
	const Value& error = find_value(reply, "error");

	if (error.type() != null_type) {
		// Error
		strPrint = "error: " + write_string(error, false);
		int code = find_value(error.get_obj(), "code").get_int();
		nRet = abs(code);
	} else {
		// Result
		if (result.type() == null_type)
			strPrint = "";
		else if (result.type() == str_type)
			strPrint = result.get_str();
		else
			strPrint = write_string(result, true);
	}
	strRet = strPrint;
	BOOST_TEST_MESSAGE(strPrint);
	//cout << strPrint << endl;
	return nRet;
}

static void CreateRegisterTx() {
	//cout <<"CreateRegisterTx" << endl;
	int argc = 5;
	const char *argv[5] = { "rpctest", "registeraccount", "dkJwhBs2P2SjbQWt5Bz6vzjqUhXTymvsGr", "0", "10" };
	CommandLineRPC(argc, const_cast<char**>(argv));
}

void CreateNormalTx() {
	//cout <<"CreateNormalTx" << endl;
	int argc = 7;
	const char *argv[7] = { "rpctest", "createnormaltx", "5zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG",
			"dkJwhBs2P2SjbQWt5Bz6vzjqUhXTymvsGr", "1000000000", "1000000", "0" };
	CommandLineRPC(argc, const_cast<char**>(argv));
}


void CreateRegScriptTx() {
	//cout <<"CreateRegScriptTx" << endl;
	int argc = 6;
	const char *argv[6] =
			{ "rpctest", "deploycontracttx", "5Vp1xpLT8D2FQg3kaaCcjqxfdFNRhxm4oy7GXyBga9",
					"fd3e0102001d000000000022220000000000000000222202011112013512013a75d0007581bf750900750a0f020017250910af08f509400c150a8008f5094002150ad2af222509c582c0e0e50a34ffc583c0e0e509c3958224f910af0885830a858209800885830a858209d2afcef0a3e520f0a37808e608f0a3defaeff0a3e58124fbf8e608f0a3e608f0a30808e608f0a3e608f0a315811581d0e0fed0e0f815811581e8c0e0eec0e022850a83850982e0a3fee0a3f5207808e0a3f608dffae0a3ffe0a3c0e0e0a3c0e0e0a3c0e0e0a3c0e010af0885820985830a800885820985830ad2afd083d0822274f8120042e990fbfef01200087f010200a8c082c083ea90fbfef0eba3f012001202010cd083d0822274f812004274fe12002ceafeebff850982850a83eef0a3eff0aa09ab0a790112013d80ea79010200e80200142200",
					"1000000", "10" };
	CommandLineRPC(argc, const_cast<char**>(argv));
}

void CreateRegScriptTxTest() {
	//cout <<"CreateRegScriptTx" << endl;
	int argc = 7;
	const char *argv[7] =
			{ "rpctest", "deploycontracttx", "5Vp1xpLT8D2FQg3kaaCcjqxfdFNRhxm4oy7GXyBga9","0","d:\\sdk.bin","1000000", "10" };
	CommandLineRPC(argc, const_cast<char**>(argv));
}

void CreateRegBetScriptTx() {
	//cout <<"CreateRegBetScriptTx" << endl;
	int argc = 6;
	const char *argv[6] =
			{ "rpctest", "deploycontracttx", "5SKNSaC9zRQ2YkL1oo8jGcgSAjNrbEZLJ3FqEkrUu2",
					"32000000000000003200000000000000640000005a000000fdd316020056000000000000000080fb000000000000000000000216d1e4900f43780779018002f0a3d8fcd9fa7a007b0f900065784479018015e493a3ad82ae838a828b83f0a3aa82ab838d828e83d8e9d9e712128f1216ce75d0007581bf75100075110f02001a353030303030303030303000313030303030303030000000000200d4ef1f11000000001b03180314004600383030303030303030300000000000000000000002000100cac0e0e6f0a308dafad0e0fa22cac0e0e0a3c582ccc582c583cdc583f0a3c582ccc582c583cdc583dae6d0e0ca22251010af08f510400c15118008f51040021511d2af2200c0d0251010af08f510500c05118008f51050020511d2afd0d0222510f58210af08f510400c15118008f51040021511d2af851183222510f582e43511f583222510f8e43511f9222510fae43511fb222510fce43511fd222510c582c0e0e51134ffc583c0e0e510c3958224f910af088583118582108008858311858210d2afcef0a3e520f0a37808e608f0a3defaeff0a3e58124fbf8e608f0a3e608f0a30808e608f0a3e608f0a315811581d0e0fed0e0f815811581e8c0e0eec0e022851183851082e0a3fee0a3f5207808e0a3f608dffae0a3ffe0a3c0e0e0a3c0e0e0a3c0e0e0a3c0e010af088582108583118008858210858311d2afd083d08222740280027401c0e0f404120107d0e01200a82274f8120144e9feee90fbfff01200087f010201aa74f512014474f91200d6eafeebff8c088d098510828511837400f0a37400f0750a05750b00780a1201eaeefceffd740412013412164b74021200ec7402120122e0c394fd502a7402120122e0f87900851082851183e8f0a3e9f08e828f83a3a882a983850882850983e8f0a3e9f0803d7402120122e064fd7033750a02750b00780a1201ea7405120122ac82ad83740212013412164b74021200ecee2403f8e43ff9850882850983e8f0a3e9f0851082851183e0faa3e0fb74071200ec7f040201aa74f7120144900f48e0703c7508ff75090378081201ea7c007dfc7a497b0f12164b74021200ec7508ff75090378081201ea7c007d007a007bfc12168874021200ec7401900f48f07a497b0f7f020201aa74f71201441202d38a088b09850882850983a882a983e82402fae439fb7f020201aa74f312014474fe1200d68a0a8b0b1202d38a0c8b0d850c08850d098510828511837400f0a37400f0ae08af09ac10ad11ee240dfae43ffb120211850a82850b83eaf0a3ebf0851082851183e0faa3e0fb74021200ec7f060201aa74f512014474fe1200d6eafeebff1202d38a088b0985080a85090b8510828511837400f0a37400f0850a08850b09ac10ad11e508240dfae43509fb1202118e828f83eaf0a3ebf0ac10ad118e828f83e0f8a3e0f9851082851183e028faa3e039fb1202118e828f83eaf0a3ebf0851082851183e0faa3e0fb74021200ec7f040201aa74f312014474fe1200d68a088b09ecfeedff1202d38a0a8b0b850a0c850b0d8510828511837400f0a37400f0850c0a850d0bac10ad11e50a240dfae4350bfb120211850882850983eaf0a3ebf0ac10ad11850882850983e0f8a3e0f9851082851183e028faa3e039fb120211850882850983eaf0a3ebf0ac10ad11850882850983e0f8a3e0f9851082851183e028faa3e039fb120211850882850983eaf0a3ebf0850882850983ee2401f8e43ff9c3e098a3e09950148508828509837400f0a37400f07a007b0002056b8508828509837400f0a37400f0ac10ad11850882850983e0f8a3e0f9851082851183e028faa3e039fb120211850882850983eaf0a3ebf0ee4f602aac10ad11850882850983e0f8a3e0f9851082851183e028faa3e039fb120211850882850983eaf0a3ebf0eef8eff9e824fffee934ffffe8497098851082851183e0faa3e0fb74021200ec7f060201aa74f312014474fe1200d68a088b09ecfeedff1202d38a0a8b0b850a0c850b0d8510828511837400f0a37400f0850c0a850d0bac10ad11e50a240dfae4350bfb120211850882850983eaf0a3ebf0ac10ad11850882850983e0f8a3e0f9851082851183e028faa3e039fb120211850882850983eaf0a3ebf0ac10ad11850882850983e0f8a3e0f9851082851183e028faa3e039fb120211850882850983eaf0a3ebf08508828509837400f0a37400f0ac10ad11850882850983e0f8a3e0f9851082851183e028faa3e039fb120211850882850983eaf0a3ebf0ac10ad11850882850983e0f8a3e0f9851082851183e028faa3e039fb120211850882850983eaf0a3ebf0eef8eff9e824fffee934ffffe849709c851082851183e0faa3e0fb74021200ec7f060201aa74f31201448a0a8b0b890890fc02e0fe7f00ee75f034a4cea8f075f000a428f875f034efa428ff90fc02e02508f085080974ff2509f508e5096033750c34750d00780c1201eaac0aad0b74032efa74fc3ffb12164b74021200ece50a2434f50a5002050bee2434feef3400ff80c07f060201aa741b6a700374006b7003790122790022c082c0838a828b83e06004790080208a828b83a3e064026004790080128a828b83a3a3e0c394034004790080027901d083d0822274f7120144e9fc7800ec6006ecc3941540047900806c8a828b83e0c39431400a8a828b83e0c3943a40047900805408e8c39c502e8808750900ea2508f582eb3509f583e0601b8a828b83e0c39430400a8a828b83e0c3943a4004790080240880ce08e8c39c50198808750900ea2508f582eb3509f583e06004790080050880e279017f020201aa74f31201448a0c8b0d8908ecfeedff740d120122e0f509e509c3950850067a017b008028e508c3950950067aff7bff801b85080a750b00780a1201eaeefceffdaa0cab0d1215f574021200ec7f060201aa74f112014474fa1200d67404120122eaf0a3ebf08c0a8d0b8510828511837400f0a37400f07e007f0075080075090074021201227400f0a3740ff0750e0c750f0f850a82850b83a3a3a3a3e0c3940240057900020996750c02750d00780c1201eae50a2405fce4350bfd740212013412164b74021200ec851082851183c3e09432a3e09400a2d265d03350057900020996851082851183c3e09465a3e09400a2d265d033400579000209967914e50a2407fae4350bfb120753e970057900020996e50a2407fae4350bfb1216b28a0c8b0dae0caf0d7402120122e0faa3e0fb1216b2eaf50c780c1201ee7403120122e0fca3e0fdeef9e50a2407fae4350bfb1207da74011200ec8a0c8b0d850c08850d09c3e5089401e5099400a2d265d033400479008046aa0eab0f1216b2eaf50c780c1201eeac0ead0feef9e50a2407fae4350bfb1207da74011200ec8a0c8b0d850c08850d09c3e5089400e5099400a2d265d033500479008002790174061200ec7f080201aac082c08374026c700374006d60047900801b8a828b83e06004790080108a828b83a3e064016004790080027901d083d08222c082c0838a828b83e0c3940340057900020a708a828b83e0640170208a828b83a3e0c394024004790080738a828b83a3a3a3e0c394644064790080628a828b83e0640270578a828b83a3e0c3940240047900804a8a828b83a3a3e0c3940240047900803a8a828b83a3e0f88a828b83a3a3e0687004790080258a828b83a3a3a3e0c394644004790080148a828b83a3a3a3a3e0c394644004790080027901d083d0822274f512014474cc1200d68a088b09ecfeedff750a34750b00780a1201ea7c007d00740212013412168874021200ec8510828511837400f074011201227402f074021201227401f074031201227400f0750a02750b00780a1201eaee2405fce43ffd7406120122aa82ab8312164b74021200ec750a14750b00780a1201eaee2407fce43ffd7408120122aa82ab8312164b74021200ec741a1201227400f0741b1201227404f0741c1201227401f0741d1201227400f0750a02750b00780a1201eaee2405fce43ffd7420120122aa82ab8312164b74021200ec750a14750b00780a1201eaee2407fce43ffd7422120122aa82ab8312164b74021200ec7901aa10ab1112069c750a34750b00780a1201ea7c007d00740212013412168874021200ec8510828511837400f074011201227402f074021201227401f074031201227401f0750a02750b00780a1201eaee2405fce43ffd7406120122aa82ab8312164b74021200ec750a14750b00780a1201eaee2407fce43ffd7408120122aa82ab8312164b74021200ec741a1201227400f0741b1201227404f0741c1201227401f0741d1201227401f0750a02750b00780a1201eaee2405fce43ffd7420120122aa82ab8312164b74021200ec750a14750b00780a1201eaee2407fce43ffd7422120122aa82ab8312164b74021200ec7901aa10ab1112069c790174341200ec7f040201aa74f3120144e9f87c00eb68fc8a087509008c0a750b008b0c750d00e50a250cfee50b350dffee65087003ef650960047900800279017f060201aa74f112014474ca1200d67434120122eaf0a3ebf0ecfeedff890d7445120122e0f50ea3e0f50f750800750c00750900850e82850f83a3a3a3e0fb850e82850f83a3e0f87900ee28f582ef39f583a3a3e0fa850e82850f83a3e0f87900ee28f582ef39f583e0f9120c77e96021850e82850f83a3e0f508850e82850f83a3e07004740180027400f50c750901801f850e82850f83a3e07004740180027400f508850e82850f83a3e0f50c750900e50d6003020f6b750a34750b00780a1201ea7c007d00740212013412168874021200ec8510828511837400f07401120122740af074021201227401f0e50cc0e07403120122d0e0f0750a02750b00780a1201eaee2405fce43ffd7406120122aa82ab8312164b74021200ec750a14750b00780a1201eaee2407fce43ffd7408120122aa82ab8312164b74021200ec741a1201227401f0741b1201227404f0e5097008741b1201227401f0741c1201227401f0e508c0e0741d120122d0e0f0750a02750b00780a1201eaee2405fce43ffd7420120122aa82ab8312164b74021200ec750a14750b00780a1201eaee2407fce43ffd7422120122aa82ab8312164b74021200ec7901aa10ab1112069ce5096003020f6b750a34750b00780a1201ea7c007d00740212013412168874021200ec8510828511837400f07401120122740af074021201227401f0e508c0e07403120122d0e0f0750a02750b00780a1201eaee2405fce43ffd7406120122aa82ab8312164b74021200ec750a14750b00780a1201eaee2407fce43ffd7408120122aa82ab8312164b74021200ec741a1201227401f0741b1201227401f0741c1201227401f0e508c0e0741d120122d0e0f0750a02750b00780a1201eaee2405fce43ffd7420120122aa82ab8312164b74021200ec750a14750b00780a1201eaee2407fce43ffd7422120122aa82ab8312164b74021200ec7901aa10ab1112069ca90974361200ec7f080201aa74f112014474c91200d67401120122eaf0a3ebf0ecfeedff7446120122e0f508a3e0f509750b00750a008510828511837400f0750f00750e0078081201ea7901eefceffd7403120122e0faa3e0fb120cb174021200ece970057900021285850882850983a3a3a3a3e0fb850882850983a3a3e0f87900ee28f582ef39f583a3a3e0fa850882850983a3a3e0f87900ee28f582ef39f583e0f9120c77e970057900021285850882850983a3e0f50b8e828f83e0f8850882850983a3a3a3e068c0e0851082851183d0e0f08e828f83a3e0f8850882850983a3a3a3a3e068f50f851082851183e0650f75f00684e5f02401f50e8e828f83a3a3a3a3e06007e50ec3940450128e828f83a3a3a3a3e0700ae50ec394045003750a01750c34750d00780c1201ea7c007d00740512013412168874021200ec74031201227400f07404120122740af074051201227401f0e50bc0e07406120122d0e0f0750c02750d00780c1201eaee2405fce43ffd7409120122aa82ab8312164b74021200ec750c14750d00780c1201eaee2407fce43ffd740b120122aa82ab8312164b74021200ec741d1201227402f0741e1201227401f0741f1201227401f0e50ac0e07420120122d0e0f0750c02750d00780c1201eaee2405fce43ffd7423120122aa82ab8312164b74021200ec750c14750d00780c1201eaee2407fce43ffd7425120122aa82ab8312164b74021200ec7901740312013412069c750c34750d00780c1201ea7c007d00740512013412168874021200ec74031201227401f07404120122740af074051201227401f0e50bc0e07406120122d0e0f0750c02750d00780c1201eaee2405fce43ffd7409120122aa82ab8312164b74021200ec750c14750d00780c1201eaee2407fce43ffd740b120122aa82ab8312164b74021200ec741d1201227402f0741e1201227401f0741f1201227401f0e50ac0e07420120122d0e0f0750c02750d00780c1201eaee2405fce43ffd7423120122aa82ab8312164b74021200ec750c14750d00780c1201eaee2407fce43ffd7425120122aa82ab8312164b74021200ec7901740312013412069c790174371200ec7f080201aa74ef1200d6740f1201227400f0a37400f0740d1201227400f0a37400f074021201227400f0a37400f08510828511837400f0a37400f0740b1201227400f0a37400f074091201227400f0a37400f07508007509007e007f00900f43740412013c74051200b51203238a0c8b0d850c0a850d0baa0aab0b12071fe9700a79001201fd79000215ef740f120134120345740b120122eaf0a3ebf0740b120122e0f8a3e0f9880e890f740f120122e0faa3e0fb12070fe9700a79001201fd79000215efac0ead0faa0aab0b12082be9700a79001201fd79000215ef740d12013412039f7409120122eaf0a3ebf0740d120122e0fca3e0fd7409120122e0faa3e0fb1209a0e9700a79001201fd79000215ef850a82850b83a3a3e0c0e07404120122d0e0f07404120122e07016ac0ead0faa0aab0b120a75e979011201fd79010215ef7404120122e0640160030214997c007d0074021201341204218a0c8b0d850c08850d097402120122e064017004a3e06400600a79001201fd79000215ef7c007d00aa10ab111205758a0c8b0dae0caf0d851082851183e064017004a3e06400600a79001201fd79000215ef8e828f83e0c0e07405120122d0e0f0850882850983e0c0e07407120122d0e0f074041201341209d2e9700a79001201fd79000215ef740412012c880c890d780c1201ea7900ac0ead0faa0aab0b120cb174021200ece979011201fd79010215ef7404120122e0640260030215e87c007d0074021201341204218a0c8b0d850c08850d097402120122e064017004a3e06400600a79001201fd79000215ef7c007d00aa10ab111205758a0c8b0dae0caf0d851082851183e064017004a3e06400600a79001201fd79000215ef8e828f83e0c0e07405120122d0e0f0850882850983e0c0e07407120122d0e0f07c017d0074021201341204218a0c8b0d850c08850d097402120122e064017004a3e06400600a79001201fd79000215ef7c017d00aa10ab111205758a0c8b0dae0caf0d851082851183e064017004a3e06400600979001201fd7900806e8e828f83e0c0e07406120122d0e0f0850882850983e0c0e07408120122d0e0f074041201341209d2e9700979001201fd7900803a740412012c880c890d780c1201eaac0ead0faa0aab0b120f7774021200ece9700979001201fd7900801079011201fd7901800779001201fd790074111200ec2274f8120144ecf8edf97408120122e0fca3e08015a3aa82ab8388828983a3a882a983ec24ff1ced34fffdec4d601f88828983e0fe8a828b83e0ffee6f60d6efc39e50067aff7bff80087a0180027a007b007f010201aa74f8120144eaf8ebf97408120122e0fea3e0801f8c828d83e088828983f0a3a882a9838c828d83a3ac82ad83ee24ff1eef34ffffee4f70dc7f010201aac082c083851082851183e0f8a3e0f9e84960128a828b83ecf0a3e824ff18e934fff94870f2d083d08222c082c0838a828b838001a3e070fce582c39afae5839bfbd083d0822202001780fe00",
					"1000000", "350" };
	CommandLineRPC(argc, const_cast<char**>(argv));
}

bool CreateBetTx1() {
	//cout << "Create bet tx1" << endl;
	std::vector<std::string> vInputParams;
	vInputParams.clear();
	vInputParams.push_back("010000000100");
	vInputParams.push_back("[]");
	vInputParams.push_back(
			"[\"5j9hghjMKwAcY33kQoSxPDJPvokt75dDeYpU5LVgUc\",\"5zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG\"]");
	vInputParams.push_back("0b434b430046003531303030303030300000000000000000000000");
	vInputParams.push_back("100000");
	vInputParams.push_back("10");
	std::string strReturn;
	if (TestCallRPC("createsecuretx", vInputParams, strReturn) > 0) {
		vInputParams.clear();
		//cout << "create secure tx succeed" << endl;
		vInputParams.push_back(strReturn);
		if (TestCallRPC("signsecuretx", vInputParams, strReturn) > 0) {
			vInputParams.clear();
			vInputParams.push_back(strReturn);
			//cout <<"sign securte tx succeed" << endl;
			if (TestCallRPC("signsecuretx", vInputParams, strReturn) > 0) {
				strSecureBetTxHash = strReturn;
				return true;
			}
		}
	}
	return false;
}

void CreateBetTx2() {
	//cout << "Create Bet tx2" << endl;
	std::vector<std::string> vInputParams;
	vInputParams.clear();
	vInputParams.push_back("00");
	vInputParams.push_back(strSecureBetTxHash);
	//strSecureTxHash = "";
	vInputParams.push_back("22");
	vInputParams.push_back("100000");
	std::string strReturn;
	if (TestCallRPC("createappealtx", vInputParams, strReturn) > 0) {
		vInputParams.clear();
		vInputParams.push_back(strReturn);
		//cout << "sign appeal tx succeed" << endl;
		TestCallRPC("signappealtx", vInputParams, strReturn);
	}
}

void CreateBetTx3() {
	//cout << "Create Bet tx2" << endl;
	std::vector<std::string> vInputParams;
	vInputParams.clear();
	vInputParams.push_back("01");
	vInputParams.push_back(strSecureBetTxHash);
	//strSecureTxHash = "";
	vInputParams.push_back("00");
	vInputParams.push_back("100000");
	std::string strReturn;
	if (TestCallRPC("createappealtx", vInputParams, strReturn) > 0) {
		vInputParams.clear();
		vInputParams.push_back(strReturn);
		//cout << "sign appeal tx succeed" << endl;
		TestCallRPC("signappealtx", vInputParams, strReturn);
	}

}

void ListRegScript() {
	//cout << "listRegScript" << endl;
	int argc = 2;
	const char *argv[2] = { "rpctest", "listapp" };
	CommandLineRPC(argc, const_cast<char**>(argv));
}

void GenerateMiner() {
	//cout <<"Generate miner" << endl;
	int argc = 3;
	const char *argv[3] = { "rpctest", "setgenerate", "true" };
	CommandLineRPC(argc, const_cast<char**>(argv));
}

bool CreateSecuretx() {
	//cout << "Create secure tx" << endl;
//	const char *argv[8] = {"rpctest", "createsecuretx", "030000000300", "[\"5Vp1xpLT8D2FQg3kaaCcjqxfdFNRhxm4oy7GXyBga9\"]", "[\"5j9hghjMKwAcY33kQoSxPDJPvokt75dDeYpU5LVgUc\",\"5zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG\"]", "01021e00070021120500000000000000", "100000", "10"};
//	CommandLineRPC(argc, const_cast<char* [5]>(argv));
	std::vector<std::string> vInputParams;
	vInputParams.clear();
	vInputParams.push_back("030000000300");
	vInputParams.push_back("[\"5Vp1xpLT8D2FQg3kaaCcjqxfdFNRhxm4oy7GXyBga9\"]");
	vInputParams.push_back(
			"[\"5j9hghjMKwAcY33kQoSxPDJPvokt75dDeYpU5LVgUc\",\"5zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG\"]");
	vInputParams.push_back("1e00070035303030303030303000a21040049e1062a4ae75");
	vInputParams.push_back("100000");
	vInputParams.push_back("10");
	std::string strReturn;
	if (TestCallRPC("createsecuretx", vInputParams, strReturn) > 0) {
		vInputParams.clear();
		//cout << "create secure tx succeed" << endl;
		vInputParams.push_back(strReturn);
		if (TestCallRPC("signsecuretx", vInputParams, strReturn) > 0) {
			vInputParams.clear();
			vInputParams.push_back(strReturn);
			//cout <<"sign securte tx succeed" << endl;
			if (TestCallRPC("signsecuretx", vInputParams, strReturn) > 0) {
				strSecureTxHash = strReturn;
				return true;
			}
		}
	}
	return false;
}

void CreateAppealtx() {
	//cout << "Create Appeal tx" << endl;
	std::vector<std::string> vInputParams;
	vInputParams.clear();
	vInputParams.push_back("01");
	vInputParams.push_back(strSecureTxHash);
	//strSecureTxHash = "";
	vInputParams.push_back("35303030303030303000d10020fc280020fc2800");
	vInputParams.push_back("100000");
	std::string strReturn;
	if (TestCallRPC("createappealtx", vInputParams, strReturn) > 0) {
		vInputParams.clear();
		vInputParams.push_back(strReturn);
		//cout <<"sign appeal tx succeed" << endl;
		TestCallRPC("signappealtx", vInputParams, strReturn);
	}
}

void CreateArbitratetx() {
	//cout << "Create Appeal tx" << endl;
	std::vector<std::string> vInputParams;
	vInputParams.clear();
	vInputParams.push_back("00");
	vInputParams.push_back(strSecureTxHash);
	//strSecureTxHash = "";
	vInputParams.push_back("353030303030303030000000c856a210741cd60058039e10741cd600100000000000000000000000");
	vInputParams.push_back("100000");
	std::string strReturn;
	if (TestCallRPC("createappealtx", vInputParams, strReturn) > 0) {
		vInputParams.clear();
		vInputParams.push_back(strReturn);
		//cout <<"sign appeal tx succeed" << endl;
		TestCallRPC("signappealtx", vInputParams, strReturn);
	}
}

void GetAccountInfo(const char *address) {
	//cout << "Get Address " << address << "INFO" << endl;
	int argc = 3;
	const char *argv[3] = { "rpctest", "getaccountinfo", address };
	CommandLineRPC(argc, const_cast<char**>(argv));
}

void DisconnectBlock(int number) {
	//cout << "disconnect block" <<endl;
	int argc = 3;
	const char *argv[3] = { "rpctest", "disconnectblock", "1" };
//	sprintf(argv[2], "%d", number);
	CommandLineRPC(argc, const_cast<char**>(argv));
}

void GetAccountState() {
	GetAccountInfo("5zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG");
	GetAccountInfo("dkJwhBs2P2SjbQWt5Bz6vzjqUhXTymvsGr");
	GetAccountInfo("5j9hghjMKwAcY33kQoSxPDJPvokt75dDeYpU5LVgUc");
}

BOOST_AUTO_TEST_SUITE(updateaccount_test)



BOOST_AUTO_TEST_CASE(get_account_info)
{
	//cout << "=====================get account info ==================================" << endl;
	GetAccountInfo("5zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG");
	GetAccountInfo("dkJwhBs2P2SjbQWt5Bz6vzjqUhXTymvsGr");
	GetAccountInfo("5j9hghjMKwAcY33kQoSxPDJPvokt75dDeYpU5LVgUc");
	GetAccountInfo("5Vp1xpLT8D2FQg3kaaCcjqxfdFNRhxm4oy7GXyBga9");
}
BOOST_AUTO_TEST_CASE(create_normaltx)
{
	CreateNormalTx();
}
BOOST_AUTO_TEST_CASE(create_regscripttx)
{
//	CreateRegScriptTx();
	CreateRegScriptTxTest();
}
BOOST_AUTO_TEST_CASE(create_registertx)
{
	CreateRegisterTx();
}
BOOST_AUTO_TEST_CASE(connect_block_test)
{
	//cout << "=====================init account info ========================" << endl;
	GetAccountState();
	//MilliSleep(1000);
	CreateNormalTx();//"5zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG" "dkJwhBs2P2SjbQWt5Bz6vzjqUhXTymvsGr"
	GenerateMiner();
	//MilliSleep(1000);
	//cout << "=====================block height 1 account info ==============" << endl;
	GetAccountState();
	GenerateMiner();
	//MilliSleep(1000);
	//cout << "=====================block height 2 account info ==============" << endl;
	GetAccountState();
	CreateNormalTx();
	CreateRegisterTx();//"dkJwhBs2P2SjbQWt5Bz6vzjqUhXTymvsGr"
	CreateRegScriptTx();//"5Vp1xpLT8D2FQg3kaaCcjqxfdFNRhxm4oy7GXyBga9"
	GenerateMiner();
	//MilliSleep(1000);
	//cout << "=====================block height 3 account info ==============" << endl;
	GetAccountState();
	//cout << "=====================list registed script info ================" << endl;
	ListRegScript();
	//cout << "=====================before create securte tx  ================" << endl;
	GetAccountState();
	CreateSecuretx();
	GenerateMiner();
	//cout << "=====================after confirm securte tx =================" << endl;
	GetAccountState();
	if ("" != strSecureTxHash) {
		//cout << "======================create appeal tx==========================" << endl;
		CreateAppealtx();
		GenerateMiner();
		//cout << "=======================after confirm appeal tx =================" << endl;
		GetAccountState();
		//cout << "=======================create arbitrate tx===========================" << endl;
		CreateArbitratetx();
		GenerateMiner();
		GetAccountState();
	}

}
BOOST_AUTO_TEST_CASE(create_securetx) {
	CreateSecuretx();
}
BOOST_AUTO_TEST_CASE(create_appealtx) {
	CreateRegScriptTx();
	GenerateMiner();
	if(CreateSecuretx()) {
		GenerateMiner();
		CreateAppealtx();
	}

}
BOOST_AUTO_TEST_CASE(bet_test) {
	//cout << "=====================init account info ========================" << endl;
	GetAccountState();
	CreateRegBetScriptTx();
	GenerateMiner();
	//cout << "=====================bet tx 1========================" << endl;
	ListRegScript();
	CreateBetTx1();
	GenerateMiner();
	GetAccountState();
	//cout << "=====================bet tx 2========================" << endl;
	CreateBetTx3();
	GenerateMiner();
	GetAccountState();
	//cout << "=====================bet tx 3 ================" << endl;
	CreateBetTx2();
	GenerateMiner();
	GetAccountState();
}
BOOST_AUTO_TEST_CASE(create_bet_script) {
	CreateRegBetScriptTx();
}

BOOST_AUTO_TEST_CASE(disconnect_block_test)
{
	DisconnectBlock(1);
	//cout << "=====================disconnetc 1 block account info ==========" << endl;
	GetAccountState();
}

BOOST_AUTO_TEST_CASE(test) {
	CreateNormalTx();
	GenerateMiner();
//	cout<< "mine 1 block, expired height 1" << endl;
	CreateNormalTx();
	GenerateMiner();
//	cout << "mine 1 block, expired height 2" << endl;
	CreateNormalTx();
	GenerateMiner();
//	cout << "mine 1 block, expired height 3" << endl;
	DisconnectBlock(1);
//	cout << "disconnect 1 block, expired height 2" << endl;
	DisconnectBlock(1);
//	cout << "disconnect 1 block, expired height 1" << endl;
	GenerateMiner();
//	cout << "mine 1 block, expired height 4" << endl;

}
/**
 * ���Խ���ͬ��׼ȷ���뼰ʱ��
 */
BOOST_AUTO_TEST_CASE(test1) {
	int index = 10000;
	while(index--) {
		CreateNormalTx();
		MilliSleep(500);
	}
}
BOOST_AUTO_TEST_SUITE_END()
