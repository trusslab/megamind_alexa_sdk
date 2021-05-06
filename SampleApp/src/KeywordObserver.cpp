/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "SampleApp/KeywordObserver.h"
#include <AIP/AudioInputProcessor.h>
//Mohammad add
#include "sock.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <thread>
#include <unistd.h>

//#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
//#include <string.h>


void report(const char* msg, int terminate) {
  perror(msg);
  if (terminate) exit(-1); /* failure */
}
void wait_on_pipe(std::string  name){
        std::string path = "/tmp/MegaMind/" + name;
        const char * myfifo = path.c_str();
        char buff[256];
        int fd = open(myfifo, O_RDONLY);
        read(fd , buff , 1);
	std::cout<<"in wait on pipe " <<name<< " " <<buff <<"\n";
        close(fd);

}
void write_to_pipe(std::string  name, std::string  datastr){
        std::string path = "/tmp/MegaMind/" + name;
        const char * myfifo = path.c_str();
        const char * data = datastr.c_str();
        int fd = open(myfifo, O_WRONLY);
        write(fd , data , strlen(data) +1 );
        close(fd);

}

void read_from_pipe_blocking(std::string  name, char * buffer , size_t size){
        std::string path = "/tmp/MegaMind/" + name;
        const char * myfifo = path.c_str();
        int fd = open(myfifo, O_RDONLY);
        read(fd , buffer , size);
        close(fd);

}
	
//Mohammad end

namespace alexaClientSDK {
namespace sampleApp {

capabilityAgents::aip::AudioProvider * static_audioProvider = NULL;
std::shared_ptr<defaultClient::DefaultClient>  static_client = NULL;
void notify_keyword_detection_over_network(avsCommon::avs::AudioInputStream::Index end_index);
void send_session_start_notice();
void wait_for_start(){
      std::cout<<"wait_for_start [0]\n";
      while(1){
         std::cout<<"wait_for_start: before going to busy wait stage\n";
	 wait_on_pipe("listen_event");
         std::cout<<"wait_for_start: after coming out of busy wait stage\n";
         notify_keyword_detection_over_network(0);
         std::string * text_cmd = new std::string("play hidden brain");
         //wait_for_MegaMind_engine_response( &text_cmd);
	 char buff[1024];
	 read_from_pipe_blocking("MegaMind_engine_response", buff, 1024);
	 text_cmd = new std::string(buff);
	 std::cout << "MJ MJ MJ\t"<<*text_cmd<<"\n";
	 *(static_audioProvider->MegaMind_text_cmd)= *text_cmd;
	 write_to_pipe("desision_is_ready","s");
         std::cout<<"should start recording\n"; 
      }
}
void wait_for_text_start_req()
{
     while(1){
	wait_on_pipe("text_start_request");
	std::cout<<"text start request recieved\n";
        send_session_start_notice();
        static_client->notifyOfTapToTalk(*static_audioProvider, avsCommon::avs::AudioInputStream::Index(100) );
     }	
}
KeywordObserver::KeywordObserver(
    std::shared_ptr<defaultClient::DefaultClient> client,
    capabilityAgents::aip::AudioProvider audioProvider,
    std::shared_ptr<esp::ESPDataProviderInterface> espProvider) :
        m_client{client},
        m_audioProvider{audioProvider},
        m_espProvider{espProvider} {
 	static_audioProvider = &m_audioProvider;
	mkfifo("/tmp/MegaMind/listen_event", 0666);
	mkfifo("/tmp/MegaMind/desision_is_ready", 0666);
	static_client = m_client;
        std::cout<<"before run the wait for start thread\n";
        std::thread th2(wait_for_start);
        th2.detach();
	std::thread th3(wait_for_text_start_req);
        th3.detach();

        
}

void notify_keyword_detection_over_network(avsCommon::avs::AudioInputStream::Index end_index){	
	write_to_pipe("keyword_detection","s");
	return;
}
void send_session_start_notice(){
  write_to_pipe("session_start","s");

}
void KeywordObserver::onKeyWordDetected(
    std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
    std::string keyword,
    avsCommon::avs::AudioInputStream::Index beginIndex,
    avsCommon::avs::AudioInputStream::Index endIndex,
    std::shared_ptr<const std::vector<char>> KWDMetadata) {
    if (endIndex != avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX &&
        beginIndex == avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX) {
        if (m_client) {
//Mohammad
            /* fd for the socket */
	    std::cout<< " HEYYYY: \t"<<beginIndex<<"\t\t" <<endIndex;
	    send_session_start_notice();
            m_client->notifyOfTapToTalk(m_audioProvider, endIndex);

        }
    } else if (
        endIndex != avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX &&
        beginIndex != avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX) {
        auto espData = capabilityAgents::aip::ESPData::EMPTY_ESP_DATA;
        if (m_espProvider) {
            espData = m_espProvider->getESPData();
        }

        if (m_client) {
            m_client->notifyOfWakeWord(m_audioProvider, beginIndex, endIndex, keyword, espData, KWDMetadata);
        }
    }
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
