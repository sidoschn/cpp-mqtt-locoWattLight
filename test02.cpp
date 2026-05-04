#include <Wt/WApplication.h>
#include <Wt/WBreak.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WLineEdit.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>

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

    Wt::WPushButton *button2 = root()->addNew<Wt::WPushButton>("Force Stop Server");

    auto greet = [this]{
      greeting_->setText("Hello there, " + nameEdit_->text());
    };

    auto stopServer = [this]{
        exit(0);
    };

    button->clicked().connect(greet);
    button2->clicked().connect(stopServer);
}

int main(int argc, char **argv)
{
    return Wt::WRun(argc, argv, [](const Wt::WEnvironment& env) {
      return std::make_unique<HelloApplication>(env);
    });
}