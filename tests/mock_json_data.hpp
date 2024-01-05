#ifndef MOCK_JSON_DAT_HPP
#define MOCK_JSON_DAT_HPP

/*
  Type: Handshake Packet

  Head : SP_HANDSHAKE = 1  
  ID : cu0000
  Body: 
*/

std::string handShakeJson = "{\"head\":1,\"id\":\"cu0000\",\"body\":{}}";

/*
  Type: Rule/template Packet

  Head : SP_DATASENT = 2  
  ID : cu0000
  Body: 
		instanceid : instance1
		algotype : 1
		data:
			sky,airtemp,humidity,wind,water,forecast,enjoysport	<-- Header
			sunny,warm,normal,strong,warm,same,yes				<-- Data Start
			cloudy,cold,high,weak,same,change,no
			rainy,normal,NaN,NaN,cool,NaN,NaN
*/

std::string templateDataJson = "{\"head\":2,\"id\":\"cu0000\",\"body\":{\"instanceid\":\"instance1\",\"algotype\":1,\"data\":\"sky,airtemp,humidity,wind,water,forecast,enjoysport\\nsunny,warm,normal,strong,warm,same,yes\\ncloudy,cold,high,weak,same,change,no\\nrainy,normal,NaN,NaN,cool,NaN,NaN\\n\"}}"; 

/*
  Type: UserData Packet

  Head : SP_DATASENT = 2  
  ID : cu0000
  Body: 
		tableid: myTable2
		priority: 2
		instancetype: instance1
		data:
			sky,airtemp,humidity,wind,water,forecast,enjoysport <-- Header
			sunny,warm,normal,strong,warm,same,yes				<-- Data Start
			sunny,warm,high,strong,warm,same,yes
			rainy,cold,high,strong,cool,change,no
			sunny,warm,high,strong,cool,change,yes
			sunny,warm,normal,strong,warm,change,yes
			sunny,warm,normal,weak,warm,same,yes
			rainy,cold,high,strong,warm,same,no
			sunny,warm,normal,weak,warm,change,yes
			rainy,cold,normal,strong,cold,same,no
			sunny,warm,normal,strong,warm,same,yes
*/

std::string userDataPacket = ;"{\"head\":2,\"id\":\"cu0000\",\"body\":{\"tableid\":\"myTable2\",\"priority\":2,\"instancetype\":\"instance1\",\"data\":\"sky,airtemp,humidity,wind,water,forecast,enjoysport\\nsunny,warm,normal,strong,warm,same,yes\\nsunny,warm,high,strong,warm,same,yes\\nrainy,cold,high,strong,cool,change,no\\nsunny,warm,high,strong,cool,change,yes\\nsunny,warm,normal,strong,warm,change,yes\\nsunny,warm,normal,weak,warm,same,yes\\nrainy,cold,high,strong,warm,same,no\\nsunny,warm,normal,weak,warm,change,yes\\nrainy,cold,normal,strong,cold,same,no\\nsunny,warm,normal,strong,warm,same,yes\"}}";

std::string mockJsonVector[3] = {
	handShakeJson,
	templateDataJson,
	userDataPacket
};


std::string workerHandshake = "{\"head\":2,\"id\":\"W000\"}";

std::string workerHandShakeAck = "{\"head\":1,\"id\":\"cu0000\",\"stats\":";

std::string workerDataAck = "{\"head\":2,\"id\":\"cu0000\"";

#endif
