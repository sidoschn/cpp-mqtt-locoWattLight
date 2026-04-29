#include <iostream>
#include "mqtt/async_client.h"
#include <string>
#include <thread>
#include <fstream>
#include <chrono>
#include "HTML.h"
#include "httplib.h"
#include <vector>

const std::string SERVER_ADDRESS("tcp://192.168.0.39:1883");
const std::string DFLT_SERVER_URI("mqtt://192.168.39:1883");
const std::string CLIENT_ID("cpp_publisher");
const std::string TOPIC("cppTest/testTopic");
const std::string SUBTOPIC("cppTest/testTopic/set");
std::vector<std::string> SUBTOPICS;
std::vector<std::string> PUBTOPICS;
//HTML::Document GLOBALHTMLDOC;
HTML::Document MAINPAGE;
std::string SMAINPAGE;

const int QOS = 1;
const int N_RETRY_ATTEMPTS = 5;


std::string DEVICEID_MONITORED = "noDevice";
std::string SOC = "0";
std::string PVPOWER = "0";

/////////////////////////////////////////////////////////////////////////////

// Callbacks for the success or failures of requested actions.
// This could be used to initiate further action, but here we just log the
// results to the console.

class action_listener : public virtual mqtt::iaction_listener
{
    std::string name_;

    void on_failure(const mqtt::token& tok) override
    {
        std::cout << name_ << " failure";
        if (tok.get_message_id() != 0)
            std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
        std::cout << std::endl;
    }

    void on_success(const mqtt::token& tok) override
    {
        std::cout << name_ << " success";
        if (tok.get_message_id() != 0)
            std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
        auto top = tok.get_topics();
        if (top && !top->empty())
            std::cout << "\ttoken topic: '" << (*top)[0] << "', ..." << std::endl;
        std::cout << std::endl;
    }

public:
    action_listener(const std::string& name) : name_(name) {}
};

/**
 * Local callback & listener class for use with the client connection.
 * This is primarily intended to receive messages, but it will also monitor
 * the connection to the broker. If the connection is lost, it will attempt
 * to restore the connection and re-subscribe to the topic.
 */


class callback : public virtual mqtt::callback, public virtual mqtt::iaction_listener

{
    // Counter for the number of connection retries
    int nretry_;
    // The MQTT client
    mqtt::async_client& cli_;
    // Options to use if we need to reconnect
    mqtt::connect_options& connOpts_;
    // An action listener to display the result of actions.
    action_listener subListener_;
    
    

    // This deomonstrates manually reconnecting to the broker by calling
    // connect() again. This is a possibility for an application that keeps
    // a copy of it's original connect_options, or if the app wants to
    // reconnect with different options.
    // Another way this can be done manually, if using the same options, is
    // to just call the async_client::reconnect() method.
    void reconnect()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        try {
            cli_.connect(connOpts_, nullptr, *this);
        }
        catch (const mqtt::exception& exc) {
            std::cerr << "Error: " << exc.what() << std::endl;
            exit(1);
        }
    }

    // Re-connection failure
    void on_failure(const mqtt::token& tok) override
    {
        std::cout << "Connection attempt failed" << std::endl;
        if (++nretry_ > N_RETRY_ATTEMPTS)
            exit(1);
        reconnect();
    }

    // (Re)connection success
    // Either this or connected() can be used for callbacks.
    void on_success(const mqtt::token& tok) override {}

    // (Re)connection success
    void connected(const std::string& cause) override
    {
        std::cout << "\nConnection success" << std::endl;
        std::cout << "\nSubscribing to topic '" << TOPIC << "'\n"
                  << "\tfor client " << CLIENT_ID << " using QoS" << QOS << "\n"
                  << "\nPress Q<Enter> to quit\n"
                  << std::endl;
        for (auto &subtop : SUBTOPICS){
        cli_.subscribe(subtop, QOS, nullptr, subListener_);
        }
    }

    // Callback for when the connection is lost.
    // This will initiate the attempt to manually reconnect.
    void connection_lost(const std::string& cause) override
    {
        std::cout << "\nConnection lost" << std::endl;
        if (!cause.empty())
            std::cout << "\tcause: " << cause << std::endl;

        std::cout << "Reconnecting..." << std::endl;
        nretry_ = 0;
        reconnect();
    }

    // Callback for when a message arrives.
    void message_arrived(mqtt::const_message_ptr msg) override
    {
        std::cout << "Message arrived" << std::endl;
        std::cout << "\ttopic: '" << msg->get_topic() << "'" << std::endl;
        std::cout << "\tpayload: '" << msg->to_string() << "'\n" << std::endl;

        std::string recTopic = msg->get_topic();
        
        if (recTopic.compare(SUBTOPICS[0]) == 0)
        {
            SOC = msg->get_payload();
            generateHtmlDoc();
        }

    }

    void delivery_complete(mqtt::delivery_token_ptr token) override {}

public:
    callback(mqtt::async_client& cli, mqtt::connect_options& connOpts)
        : nretry_(0), cli_(cli), connOpts_(connOpts), subListener_("Subscription")
    {
    }

    std::string generateHtmlDoc(){
        HTML::Document htmlDoc("TitleMessage");
        std::string htmlString;
        htmlDoc.addAttribute("lang", "en");

        htmlDoc << HTML::Header2("Listening to Device: "+ DEVICEID_MONITORED) << HTML::Break();



        htmlDoc << (HTML::Table()
                <<  (HTML::Row() <<  HTML::ColHeader("SOC")   << HTML::ColHeader("PvPower"))
                <<  (HTML::Row() <<  HTML::Col(SOC)           << HTML::Col(PVPOWER)));
        htmlString = htmlDoc;
        return htmlString;
        
    }
};

/////////////////////////////////////////////////////////////////////////////


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

// self written classes::

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


const httplib::Request& respondToHi(const httplib::Request&, httplib::Response& res){
    res.set_content(SMAINPAGE, "text/html");

}


int main()
{
    // mqtt stuff init

    mqtt::async_client cli(DFLT_SERVER_URI, CLIENT_ID);
    mqtt::connect_options connOpts;
    connOpts.set_clean_session(false);
    SUBTOPICS.push_back("cppTest/testTopic/set");
    SUBTOPICS.push_back("cppTest/testTopic2/set");

    callback cb(cli, connOpts);


    //webserver stuff init
    httplib::Server svr;
    // std::ofstream fileStream;
    
    
    // fileStream.open("newHtml.htm");
    
    // generate a simple HTML doc to show on /hi request
    SMAINPAGE = cb.generateHtmlDoc();

    // HTML::Document htmlDoc("TitleMessage");
    // std::string htmlString;
    // htmlDoc.addAttribute("lang", "en");

    // htmlDoc << HTML::Header2("Listening to Device: "+ deviceId) << HTML::Break();



    // htmlDoc << (HTML::Table()
    //         <<  (HTML::Row() <<  HTML::ColHeader("SOC")   << HTML::ColHeader("PvPower"))
    //         <<  (HTML::Row() <<  HTML::Col(SoC)           << HTML::Col(pvPower)));


    // std::cout << htmlDoc;

    // fileStream << htmlDoc;
    // fileStream.close();
    // htmlString = htmlDoc;
    
    svr.Get("/hi", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(SMAINPAGE, "text/html");
        });
    httplib::Server::Handler serverHandler;

    

    
    svr.Get("/hi", serverHandler);

        
    svr.Get("/stop", [&svr](const httplib::Request&, httplib::Response& res) {
        std::cout << "stopping server" << std::endl;
        svr.stop();

        });

    //setting mqtt callbacks and connecting to broker
    
    cli.set_callback(cb);
    try {
        std::cout << "Connecting to the MQTT server '" << DFLT_SERVER_URI << "'..." << std::flush;
        cli.connect(connOpts, nullptr, cb);
    }
    catch (const mqtt::exception& exc) {
        std::cerr << "\nERROR: Unable to connect to MQTT server: '" << DFLT_SERVER_URI << "'" << exc
                  << std::endl;
        return 1;
    }
    

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
