#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>
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

string appKey/* = "oI8amHfNKhLvO87K"*/;
string accessToken/* = "18de77cbf3444c2390465fe3f6de2c7e"*/;
string vrfile{""};
uintmax_t contentLen/* = 94694*/;

void connect_to_file(iostream& s, const string &server, int& sz);

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
  //cout << "accessToken: " << accessToken << "\nappKey: " << appKey << endl;
  
  // 2. Read media file
  if (argc == 2) {
    vrfile = string(argv[1]);
  } else {
    vrfile = "nls-sample-16k.wav"; 
  }
  //boost::filesystem::path media_path = boost::filesystem::current_path() / vrfile;
  //contentLen = boost::filesystem::file_size(media_path);
  fstream vrfstr(vrfile, ios::in | ios::binary);
  long start = vrfstr.tellg();
  vrfstr.seekg(0, ios::end);
  long end = vrfstr.tellg();
  vrfstr.close();
  contentLen = end - start;

  //cout << to_string(contentLen) << endl;
  //return 0;

  // 3. Connect to Server
  try {
    string server = "nls-gateway.cn-shanghai.aliyuncs.com";
    boost::asio::ip::tcp::iostream s{server, "http"};
    int resp_body_size = 0;
    connect_to_file(s, server, resp_body_size);

	auto data = std::make_unique<char[]>(resp_body_size);
	s.read(data.get(), resp_body_size);
    s.close();

	string resp_str;
	std::copy(data.get(), data.get() + resp_body_size, back_inserter(resp_str));
#ifdef _WIN32
	resp_str = boost::locale::conv::between(resp_str, "UTF-8", "GB2312");
#endif
	cout << resp_str << endl;
    stringstream ss;
    ss << resp_str;

    boost::property_tree::ptree tree;
    boost::property_tree::read_json(ss, tree);
    string task_id = tree.get<string>("task_id");
    string result  = tree.get<string>("result");
    int    status  = tree.get<int>("status");
    string message = tree.get<string>("message");

    cout << "task_id:" << task_id << ", result: " << result << ", status: " << status << ", message: " << message << endl; 
  } catch (std::exception &e) {
    cout << "Exception:" << e.what() << "\n";
    return 1;
  }

  return 0;
}

void connect_to_file(iostream& s, const string &server, int& len)
{
  if (!s)
    throw runtime_error("can't connect\n");

  s << "POST " << "/stream/v1/asr?appkey=" + appKey + "&sample_rate=8000" + " HTTP/1.1\r\n";
  s << "HOST:" << "nls-gateway.cn-shanghai.aliyuncs.com\r\n";
  s << "Content-Length:" << to_string(contentLen) << "\r\n";
  s << "Content-type:application/octet-stream\r\n";
  s << "X-NLS-Token:" << accessToken << "\r\n";
  s << "Accept-Encoding: gzip" << "\r\n";
  s << "\r\n";

  fstream ofstr(vrfile.c_str());
  auto t0 = chrono::system_clock::now();
  //s << ofstr.rdbuf();
  auto buff = std::make_unique<char[]>(contentLen);
  ofstr.read(buff.get(), contentLen);
  s.write(buff.get(), contentLen);
  s.flush();
  auto t1 = chrono::system_clock::now();
  ofstr.close();

  string http_version;
  unsigned int status_code;
  auto t2 = chrono::system_clock::now();
  s >> http_version >> status_code;
  auto t3 = chrono::system_clock::now();
  string status_message;
  getline(s, status_message);

  cout << "request  cost: " << chrono::duration_cast<chrono::milliseconds>(t1 - t0).count() << " milliseconds\n"
	  << "response cost: " << chrono::duration_cast<chrono::milliseconds>(t3 - t2).count() << " milliseconds\n";

  if (!s || http_version.substr(0, 5) != "HTTP/")
      throw runtime_error{"Invalid response\n"};

  if (status_code != 200)
    throw runtime_error{"Response returned with status code"};

  string header;
  while (getline(s, header) && header !="\r") {
    if ( header.substr(0, 14) == "Content-Length" ) {
      string::size_type lenEnd = header.find('\r');
      len = stoi(header.substr(16, lenEnd-16)); 
      //std::cout << "len:" << len << endl;
    }
  }
}
