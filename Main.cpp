#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
//#include <boost/filesystem.hpp>
#include <boost/locale.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <stdexcept>
#include <memory>
#include <functional>
#include <iterator>

using namespace std;

string appKey;
string accessToken;

void http_connect(shared_ptr<iostream> s, const string& vrfile, long content_len, int& sz);

int main(int argc, char *argv[])
{
  // 1. Read Conf
  string conf("./conf/VRServiceEngine.xml");
  boost::property_tree::ptree conftree;
  boost::property_tree::read_xml(conf, conftree);
  for (const auto &node : conftree.get_child("VRServiceEngine")) {
    if (node.first == "<xmlattr>") {
      string MakeTestLog = node.second.get<string>("MakeTestLog");
      string Debug       = node.second.get<string>("Debug");
      string EngineType  = node.second.get<string>("EngineType");
             accessToken = node.second.get<string>("AppId");
             appKey      = node.second.get<string>("appKey");
    }
  }
  cout << "accessToken: " << accessToken << "\nappKey: " << appKey << endl;

  string server("nls-gateway.cn-shanghai.aliyuncs.com");
  auto s = make_shared<boost::asio::ip::tcp::iostream>(server, "http");
  
  int counter{0};
  for (;;) {
    cout << "[ " << counter++ << " ] WavName: ";
    string vrfile;
    getline(cin, vrfile);
    if (vrfile.empty()) {
      cout << "empty, QUIT!\n";
      break;
    }
    // 2. Read media file
    //boost::filesystem::path media_path = boost::filesystem::current_path() / vrfile;
    //content_len = boost::filesystem::file_size(media_path);
    fstream vrfstrm(vrfile, ios::in | ios::binary);
    long content_len = 0;
    long start = vrfstrm.tellg();
    vrfstrm.seekg(0, ios::end);
    long end = vrfstrm.tellg();
    vrfstrm.close();
    content_len = end - start;
    if (content_len == 0) {
      cout << "doesn't exists file!\n";
      continue;
    }
    //cout << to_string(content_len) << endl;
    //return 0;

    // 3. Connect to Server
    try {
      int body_len = 0;
      // yes, content_len-58, we should remove wav header
      http_connect(s, vrfile, content_len-78, body_len);

      // Http Response: Parse Body
      auto body = std::make_unique<char[]>(body_len);
      s->read(body.get(), body_len);

      string resp_str;
      std::copy(body.get(), body.get() + body_len, back_inserter(resp_str));
      stringstream ss;
      ss << resp_str;

      boost::property_tree::ptree tree;
      boost::property_tree::read_json(ss, tree);
      string task_id = tree.get<string>("task_id");
      string result  = tree.get<string>("result");
      int    status  = tree.get<int>("status");
      string message = tree.get<string>("message");

#ifdef _WIN32
      result = boost::locale::conv::between(result, "GBK", "UTF-8");
#endif
      cout << "Response:\n{\n\t\"task_id\":\"" << task_id 
        << "\", \n\t\"result\": \"" << result 
        << "\", \n\t\"status\": " << status 
        << ",\n\t\"message\": \"" << message
        << "\"\n}" << endl << endl;
    } catch (std::exception &e) {
      cout << "Exception:" << e.what() << "\n";
      return 1;
    }
  }
  getchar();

  return 0;
}

void http_connect(shared_ptr<iostream> s, const string& vrfile, long content_len, int& body_len)
{
  if (!s)
    throw runtime_error("can't connect\n");

  // Http Request: SetHeader
  *s << "POST " << "/stream/v1/asr?appkey=" + appKey + "&sample_rate=16000" + " HTTP/1.1\r\n";
  *s << "HOST:" << "nls-gateway.cn-shanghai.aliyuncs.com\r\n";
  *s << "Content-Length:" << to_string(content_len) << "\r\n";
  *s << "Content-type:application/octet-stream\r\n";
  *s << "X-NLS-Token:" << accessToken << "\r\n";
  *s << "Accept-Encoding: gzip" << "\r\n";
  *s << "\r\n";

  // Http Request: SetBody
  fstream ofstrm(vrfile.c_str(), ios::in | ios::binary);
  // Skip Wav Header
  ofstrm.seekg(78);
  //auto t0 = chrono::system_clock::now();
  auto buff = std::make_unique<char[]>(content_len);
  ofstrm.read(buff.get(), content_len);
  s->write(buff.get(), content_len);
  s->flush();
  //auto t1 = chrono::system_clock::now();
  ofstrm.close();

  // Http Response: Parse Header
  string http_version;
  unsigned int status_code;
  //auto t2 = chrono::system_clock::now();
  *s >> http_version >> status_code;
  //auto t3 = chrono::system_clock::now();
  string status_message;
  getline(*s, status_message);

  //cout << "request  costs: " << chrono::duration_cast<chrono::milliseconds>(t1 - t0).count() << " milliseconds\n"
	  // << "response costs: " << chrono::duration_cast<chrono::milliseconds>(t3 - t2).count() << " milliseconds\n";

  if (!s || http_version.substr(0, 5) != "HTTP/")
    throw runtime_error{"Invalid response\n"};

  //if (status_code != 200)
  //  throw runtime_error{"Response returned with status code " + to_string(status_code)};

  string header;
  while (getline(*s, header) && header !="\r") {
    if ( header.substr(0, 14) == "Content-Length" ) {
      string::size_type end_pos = header.find('\r');
      body_len = stoi(header.substr(16, end_pos-16)); 
      //std::cout << "len:" << len << endl;
    }
  }

  if (status_code != 200) {
	  auto body = std::make_unique<char[]>(body_len);
	  s->read(body.get(), body_len);

	  string resp_str;
	  std::copy(body.get(), body.get() + body_len, back_inserter(resp_str));
#ifdef _WIN32
	  string gbkResult = boost::locale::conv::between(resp_str, "GBK", "UTF-8");
	  cout << gbkResult << endl;
#endif

	  throw runtime_error{ "Response returned with status code " + to_string(status_code) };
  }
}
