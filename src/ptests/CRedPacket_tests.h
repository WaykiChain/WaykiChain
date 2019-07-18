
#include "CycleTestBase.h"
#include <stdlib.h>
#include <time.h>
#include "main.h"
#include "miner/miner.h"
#include "commons/uint256.h"
#include "commons/util.h"
#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

#include "json/json_spirit_writer_template.h"
#include "./rpc/core/rpcclient.h"
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_reader.h"
#include "json/json_spirit_writer.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_stream_reader.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
using namespace std;
using namespace boost;
using namespace json_spirit;



class CRedPacketTest: public CycleTestBase {
	int nNum;
	int nStep;
	string txid;
	string strAppRegId;
	string redHash;
	string appaddr;
	uint64_t specailmM;
	string rchangeaddr;
public:
	CRedPacketTest();
	~CRedPacketTest(){};
	virtual TEST_STATE Run() ;
	bool RegistScript();
	bool WaitRegistScript();
	bool WithDraw();
	bool SendRedPacketTx();
	bool AcceptRedPacketTx();
	bool WaitTxConfirmedPackage(string txid);
	void Initialize();
	bool SendSpecailRedPacketTx();
	bool AcceptSpecailRedPacketTx();
};
