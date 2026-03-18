#include "entry.hpp"
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include <httplib.h>

using namespace httplib;
using namespace ns_entry;

int main(int argv,char* argc[])
{
    if(argv != 2)
    {
        std::cout<<"format: ./" << __FILE__ << " port";
        return 1;
    }
    HandlerEntry entry;
    Server svr;
    svr.Get("/compile_and_run", [&entry](const Request &req, Response &rep){
        std::string in_json = req.body;
        if(!in_json.empty())
        {
            std::string out_json = entry.Run(in_json);
            rep.set_content(out_json, "application/json;charset=utf-8");
        }
    });
    svr.listen("0.0.0.0", atoi(argc[1]));
    return 0;
}