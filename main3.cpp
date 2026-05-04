#include <iostream>
#include "mqtt/async_client.h"
#include <string>
#include <thread>
#include <fstream>
#include <chrono>
#include "HTML.h"
#include "httplib.h"
#include <vector>
#include "json.hpp"
#include <Wt/WApplication.h>
#include <Wt/WBreak.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WLineEdit.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>


int WEBSERVERPORT(8234);
const std::string DFLT_SERVER_URI("mqtt://192.168.39:1883");
const std::string CLIENT_ID("cpp_publisher");
const std::string TOPIC("cppTest/testTopic");
// const std::string SUBTOPIC("cppTest/testTopic/set"); // legacy, the subtopics are populated in Main
std::vector<std::string> SUBTOPICS;
std::vector<std::string> PUBTOPICS;
//HTML::Document GLOBALHTMLDOC;
HTML::Document MAINPAGE;
std::string SMAINPAGE;
int LOCALPORT;
std::string SERVER_CURRENT_LOCALADDR;

const std::string DEVICETOWATCH = "DLP0DYT037";

const int QOS = 1;
const int N_RETRY_ATTEMPTS = 5;

mqtt::async_client LOCOMQTTCLIENT(DFLT_SERVER_URI, CLIENT_ID);


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
    
    std::map<std::string , std::string> monitoredVariables;

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
        //std::cout << "\tpayload: '" << msg->to_string() << "'\n" << std::endl;

        std::string recTopic = msg->get_topic();
        
        if (recTopic.compare(SUBTOPICS[0]) == 0)
        {
            std::string jsonString = msg->get_payload();
            
            nlohmann::json jsonData = nlohmann::json::parse(jsonString);
            //JS::ParseContext context(jsonString);
            //nlohmann::json jsonData = nlohmann::json::parse(jsonMsg["data"]);
            //JsonObject obj;
            std::cout << jsonData["data"]["bms_soc"];

            SOC = nlohmann::to_string(jsonData["data"]["bms_soc"]);
            PVPOWER = std::to_string(std::stod(nlohmann::to_string(jsonData["data"]["pvpowerin"])) /10.0);
            DEVICEID_MONITORED = jsonData["device"];
            // for (auto& el : jsonData["data"].items()) {
            // std::cout << el.key() << " : " << el.value() << "\n";
            // }


            SMAINPAGE = generateHtmlDoc();
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

        // !! hier dynamisch die Übersicht generieren

        htmlDoc << HTML::Table();

        for(auto variable: monitoredVariables){
            htmlDoc << (HTML::Row() <<  HTML::ColHeader(variable.first)   << HTML::Col(variable.second));
        }


                
        htmlDoc << HTML::Break() << HTML::Break();
        htmlDoc << HTML::Link("Stop Server", "stop").title("Klick here to stop the Webserver");
        htmlString = htmlDoc;
        return htmlString;
        
    }
};

/////////////////////////////////////////////////////////////////////////////


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


struct my_Requesthandler{

    void operator()(const httplib::Request& req, httplib::Response& res){
        std::cout << "hi request received";
        res.set_content("oh, hi there", "text/plain");
    }

};


void myRequestfunction(const httplib::Request& req, httplib::Response& res){
        std::cout << "hi request received";
        res.set_content("oh, hi there", "text/plain");
    }

class HelloApplication : public Wt::WApplication
{
public:
    HelloApplication(const Wt::WEnvironment& env);

private:
    Wt::WLineEdit *nameEdit_;
    Wt::WText *greeting_;
};

HelloApplication::HelloApplication(const Wt::WEnvironment& env)
    : Wt::WApplication(env)
{
    setTitle("Hello world");

    root()->addNew<Wt::WText>("Your name, please? ");
    nameEdit_ = root()->addNew<Wt::WLineEdit>();
    Wt::WPushButton *button = root()->addNew<Wt::WPushButton>("Greet me.");
    root()->addNew<Wt::WBreak>();
    greeting_ = root()->addNew<Wt::WText>();
    root()->addNew<Wt::WBreak>();
    Wt::WPushButton *button2 = root()->addNew<Wt::WPushButton>("Force Stop WebServer");
    root()->addNew<Wt::WBreak>();
    Wt::WPushButton *button3 = root()->addNew<Wt::WPushButton>("SendMQTTtestMessage");

    auto greet = [this]{
      greeting_->setText("Hello there, " + nameEdit_->text());
    };

    auto stopServer = [this]{
        exit(0);
    };

    auto sendMessage = [this]{
        std::string payload = "Hello, EMQX from C++!";
        mqtt::message_ptr pubmsg = mqtt::make_message(TOPIC, payload, 1, false);
        try{
            LOCOMQTTCLIENT.publish(pubmsg)->wait();
        }catch(const mqtt::exception& exc){
            std::cout << "no mqtt message sent";
        }
        
    };

    button->clicked().connect(greet);
    button2->clicked().connect(stopServer);
    button3->clicked().connect(sendMessage);
}


int main(int argc, char **argv)
{
    // mqtt stuff init

    //mqtt::async_client cli(DFLT_SERVER_URI, CLIENT_ID);
    mqtt::connect_options connOpts;
    connOpts.set_clean_session(false);
    SUBTOPICS.push_back("energy/growatt/DLP0DYT037");
    SUBTOPICS.push_back("cppTest/testTopic2/set");

    callback cb(LOCOMQTTCLIENT, connOpts);

    //setting mqtt callbacks and connecting to broker
    
    LOCOMQTTCLIENT.set_callback(cb);
    try {
        std::cout << "Connecting to the MQTT server '" << DFLT_SERVER_URI << "'..." << std::flush;
        LOCOMQTTCLIENT.connect(connOpts, nullptr, cb);
    }
    catch (const mqtt::exception& exc) {
        std::cerr << "\nERROR: Unable to connect to MQTT server: '" << DFLT_SERVER_URI << "'" << exc
                  << std::endl;
        return 1;
    }
    
    
    return Wt::WRun(argc, argv, [](const Wt::WEnvironment& env) {
      return std::make_unique<HelloApplication>(env);
    });
    
    
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
