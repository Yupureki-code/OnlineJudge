// #include "../include/oj_log.hpp"

// using namespace ns_oj_log;

// int main(int argc, char* argv[])
// {
//     std::string host = "0.0.0.0";
//     int port = 8100;

//     const char* env_port = std::getenv("OJ_LOG_PORT");
//     if (env_port != nullptr)
//     {
//         try
//         {
//             port = std::stoi(env_port);
//         }
//         catch (...)
//         {
//             port = 8100;
//         }
//     }

//     if (argc >= 2)
//     {
//         try
//         {
//             port = std::stoi(argv[1]);
//         }
//         catch (...)
//         {
//             logger(ns_log::WARNING) << "invalid cli port, fallback to " << port;
//         }
//     }

//     ns_oj_log::LogServer server;
//     server.Start(host, port);
//     return 0;
// }