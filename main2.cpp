#include <iostream>
#include "mqtt/async_client.h"
#include <string>
#include <thread>
#include <fstream>
#include <chrono>
#include "HTML.h"
#include "httplib.h"

const std::string SERVER_ADDRESS("tcp://192.168.0.39:1883");
const std::string CLIENT_ID("cpp_publisher");
const std::string TOPIC("cppTest/testTopic");

// class mqttPublisher{
//     std::string brokerAddress;
//     std::string clientId;
//     mqtt::connect_options connOpts;
//     mqtt::async_client client;

//     public:
//         mqttPublisher(std::string bA, std::string cI, int keepAliveInterval, bool bSetCleanSession){  //constructor
//             brokerAddress= bA;
//             clientId = cI;
//             connOpts.set_keep_alive_interval(keepAliveInterval);
//             connOpts.set_clean_session(bSetCleanSession);
            
//         }

//     void init(){
//         //mqtt::async_client clientt(brokerAddress, clientId);
//         client.
//     }

//     int publish(std::string topic, std::string payloadToPublish){
//         try {
//         // Connect to broker
//         client.connect(connOpts)->wait();
//         std::cout << "Connected to broker" << std::endl;

//         // Publish a message
//         //std::string payload = "Hello, EMQX from C++!";

//         mqtt::message_ptr pubmsg = mqtt::make_message(topic, payloadToPublish, 1, false);
//         client.publish(pubmsg)->wait();
//         std::cout << "Message published: " << payload << std::endl;

//         // Disconnect
//         client.disconnect()->wait();
//         std::cout << "Disconnected" << std::endl;
//         } catch (const mqtt::exception& exc) {
//         std::cerr << "Error: " << exc.what() << std::endl;
//         return 1;
//         }
//         return 0;

//     }

// };


int publishAMessage(mqtt::async_client& client, mqtt::connect_options& connOpts, std::string payload)
{
       try {
        // Connect to EMQX broker
        client.connect(connOpts)->wait();
        std::cout << "Connected to EMQX broker" << std::endl;

        // Publish a message
        //std::string payload = "Hello, EMQX from C++!";
        mqtt::message_ptr pubmsg = mqtt::make_message(TOPIC, payload, 1, false);
        client.publish(pubmsg)->wait();
        std::cout << "Message published: " << payload << std::endl;

        // Disconnect
        client.disconnect()->wait();
        std::cout << "Disconnected" << std::endl;
        return 0;
    } catch (const mqtt::exception& exc) {
        std::cerr << "Error: " << exc.what() << std::endl;
        return 1;
    }
}



int main()
{
    httplib::Server svr;
    // std::ofstream fileStream;
    std::string deviceId = "noDevice";
    std::string SoC = "0";
    std::string pvPower = "0";
    
    // fileStream.open("newHtml.htm");

    HTML::Document htmlDoc("TitleMessage");
    std::string htmlString;
    htmlDoc.addAttribute("lang", "en");

    htmlDoc << HTML::Header2("Listening to Device: "+ deviceId) << HTML::Break();



    htmlDoc << (HTML::Table()
            <<  (HTML::Row() <<  HTML::ColHeader("SOC")   << HTML::ColHeader("PvPower"))
            <<  (HTML::Row() <<  HTML::Col(SoC)           << HTML::Col(pvPower)));


    std::cout << htmlDoc;

    // fileStream << htmlDoc;
    // fileStream.close();
    htmlString = htmlDoc;
    
    svr.Get("/hi", [&htmlDoc](const httplib::Request&, httplib::Response& res) {
        res.set_content(htmlDoc, "text/html");
        });

    svr.listen("0.0.0.0", 8234);
    
    // mqtt::async_client client(SERVER_ADDRESS, CLIENT_ID);

    // mqtt::connect_options connOpts;
    // connOpts.set_keep_alive_interval(20);
    // connOpts.set_clean_session(true);
    

    // std::cout << "Hello, 8 World!" << std::endl;
    // int publishResult;

    // publishResult = publishAMessage(client, connOpts, "testMessage1");

    // publishResult = publishAMessage(client, connOpts, "testMessage2");
    
    
    
    // try {
    //     // Connect to EMQX broker
    //     client.connect(connOpts)->wait();
    //     std::cout << "Connected to broker" << std::endl;

    //     // Publish a message
    //     std::string payload = "Hello !!!";
    //     mqtt::message_ptr pubmsg = mqtt::make_message(TOPIC, payload, 1, false);
    //     client.publish(pubmsg)->wait();
    //     std::cout << "Message published: " << payload << std::endl;

    //     // Disconnect
    //     client.disconnect()->wait();
    //     std::cout << "Disconnected" << std::endl;
    //     return 0;
    // } catch (const mqtt::exception& exc) {
    //     std::cerr << "Error: " << exc.what() << std::endl;
    //     return 1;
    // }
    

    return 0;
}





// mqtt::async_client client(SERVER_ADDRESS, CLIENT_ID);

//     mqtt::connect_options connOpts;
//     connOpts.set_keep_alive_interval(20);
//     connOpts.set_clean_session(true);
    
//     try {
//         // Connect to EMQX broker
//         client.connect(connOpts)->wait();
//         std::cout << "Connected to EMQX broker" << std::endl;

//         // Publish a message
//         std::string payload = "Hello, EMQX from C++!";
//         mqtt::message_ptr pubmsg = mqtt::make_message(TOPIC, payload, 1, false);
//         client.publish(pubmsg)->wait();
//         std::cout << "Message published: " << payload << std::endl;

//         // Disconnect
//         client.disconnect()->wait();
//         std::cout << "Disconnected" << std::endl;
//     } catch (const mqtt::exception& exc) {
//         std::cerr << "Error: " << exc.what() << std::endl;
//         return 1;
//     }
