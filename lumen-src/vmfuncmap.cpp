#include "lumen-inc/vm.h"

#include "widget.h"
#include <condition_variable>
#include <QThread> // TODO:  temp
#include "scriptingpanel.h"
#include <QInputDialog>

#include <set>
#include <random>
#include "lumen-inc/httplib.h"

std::set<std::string> capabilitySet = {
    "FS", "random", "HTTP"
};

int fileHandleId = 0;
std::unordered_map<int, std::fstream*> fileHandles;

static std::mt19937 rngEngine(std::random_device{}());

std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

std::unordered_map<int, NativeFn> funcMap = {
    {0x01, [](VMExecutionData* execData) {
        // println
        auto arg0 = execData->stack.back(); execData->stack.pop_back();
        std::visit([](const auto& val) {
            std::ostringstream oss;
            oss << val;
            ScriptingPanel::appendOutput(QString::fromStdString(oss.str()));
        }, arg0.data);
        ScriptingPanel::appendOutput("\n");
    }},
    {0x02, [](VMExecutionData* execData) {
        // print
        auto arg0 = execData->stack.back(); execData->stack.pop_back();
        std::visit([](const auto& val) {
            std::ostringstream oss;
            oss << val;
            ScriptingPanel::appendOutput(QString::fromStdString(oss.str()));
        }, arg0.data);
    }},
    {0x03, [](VMExecutionData* execData) {
         // inputInt
         auto varIndex = getInt(execData->stack.back());
         execData->stack.pop_back();

         std::mutex mtx;
         std::condition_variable cv;
         bool done = false;
         int result = 0;

         {
             std::unique_lock lock(mtx);

             QMetaObject::invokeMethod(
                 Widget::instance,
                 [&] {
                     bool ok;

                     int value = QInputDialog::getInt(
                         Widget::instance,
                         "Script Input",
                         "Enter a number:",
                         0,
                         -2147483647,
                         2147483647,
                         1,
                         &ok
                         );

                     {
                         std::lock_guard inner(mtx);
                         result = ok ? value : 0;
                         done = true;
                     }

                     cv.notify_one();
                 },
                 Qt::QueuedConnection
                 );

             cv.wait(lock, [&] {
                 return done;
             });
         }

         execData->variables[varIndex].type = TAG_INT;
         execData->variables[varIndex].data = static_cast<int64_t>(result);
     }},
    {0x04, [](VMExecutionData* execData) {
        // inputStr
        auto varIndex = getInt(execData->stack.back()); execData->stack.pop_back();

        std::mutex mtx;
        std::condition_variable cv;
        bool done = false;
        QString result;

        {
            std::unique_lock lock(mtx);
            done = false;

            QMetaObject::invokeMethod(
                Widget::instance,
                [&] {
                    bool ok;
                    auto value = QInputDialog::getText(
                        Widget::instance,
                        "Script Input",
                        "Enter value:",
                        QLineEdit::Normal,
                        "",
                        &ok
                        );

                    {
                        std::lock_guard inner(mtx);
                        result = ok ? value : "";
                        done = true;
                    }

                    cv.notify_one();
                },
                Qt::QueuedConnection
                );

            cv.wait(lock, [&] {
                return done;
            });
        }

        execData->variables[varIndex].type = TAG_STRING;
        execData->variables[varIndex].data = result.toStdString();
    }},
    {0x05, [](VMExecutionData* execData) {
         // str2int
         auto varIndex = getInt(execData->stack.back()); execData->stack.pop_back();
         auto value = execData->stack.back(); execData->stack.pop_back();

         int num = 0;
         std::string str = "0";
         if(value.type == TAG_STRING) str = std::get<std::string>(value.data);

         try {
             num = std::stoi(str);
         } catch (const std::invalid_argument& e) {
             num = 0;
         } catch (const std::out_of_range& e) {
             num = 0;
         }

         execData->variables[varIndex].type = TAG_INT;
         execData->variables[varIndex].data = num;
     }},
    {0x06, [](VMExecutionData* execData) {
         // int2str
         auto varIndex = getInt(execData->stack.back()); execData->stack.pop_back();
         auto value = execData->stack.back(); execData->stack.pop_back();

         int num = 0;
         if(value.type == TAG_INT) num = getInt(value);

         std::string str = std::to_string(num);
         execData->variables[varIndex].type = TAG_STRING;
         execData->variables[varIndex].data = str;
     }},
    {0x07, [](VMExecutionData* execData) {
         // str2float
         auto varIndex = getInt(execData->stack.back()); execData->stack.pop_back();
         auto value = execData->stack.back(); execData->stack.pop_back();

         double num = 0.0;
         std::string str = "0";
         str = std::get<std::string>(value.data);

         try {
             num = std::stod(str);
         } catch (const std::invalid_argument& e) {
             num = 0.0;
         } catch (const std::out_of_range& e) {
             num = 0.0;
         }

         execData->variables[varIndex].type = TAG_FLOAT;
         execData->variables[varIndex].data = num;
     }},
    {0x08, [](VMExecutionData* execData) {
         // float2str
         auto varIndex = getInt(execData->stack.back()); execData->stack.pop_back();
         auto value = execData->stack.back(); execData->stack.pop_back();

         double num = 0.0;
         if(value.type == TAG_FLOAT) num = std::get<double>(value.data);
         else if(value.type == TAG_INT) num = static_cast<double>(getInt(value)); // accept int too, same leniency as int2str only handling its own type

         std::string str = std::to_string(num);
         execData->variables[varIndex].type = TAG_STRING;
         execData->variables[varIndex].data = str;
     }},
    // stdlib impl
    {0xA0, [](VMExecutionData* execData) {
         // assertCapability
         auto value = execData->stack.back(); execData->stack.pop_back();
         if(value.type != TAG_STRING) {
             throw std::runtime_error("assertCapability failed: invalid value type");
         }
         auto str = std::get<std::string>(value.data);
         auto it = capabilitySet.find(str);
         if(it == capabilitySet.end()) {
             std::stringstream ss;
             ss << "assertCapability failed: capability " << str << " is not present";
             throw std::runtime_error(ss.str());
         }

         // capability present, proceed with execution
     }},
    {0xA1, [](VMExecutionData* execData) {
         // openFile
         auto handleVarIndex = getInt(execData->stack.back()); execData->stack.pop_back();

         auto value = execData->stack.back(); execData->stack.pop_back();

         auto filename = std::get<std::string>(value.data);

         auto stream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::trunc);
         if(!stream->is_open()) {
             throw std::runtime_error("openFile failed: unable to open file " + filename);
         }

         fileHandles[fileHandleId] = stream;

         execData->variables[handleVarIndex].type = TAG_INT;
         execData->variables[handleVarIndex].data = fileHandleId++;
     }},
    {0xA2, [](VMExecutionData* execData) {
         // writeFile
         auto handle = getInt(execData->stack.back()); execData->stack.pop_back();

         auto value = execData->stack.back(); execData->stack.pop_back();

         auto valueToWrite = std::get<std::string>(value.data);

         auto it = fileHandles.find(handle);
         if(it != fileHandles.end()) {
             auto f = it->second;
             *f << valueToWrite;
         } else {
             throw std::runtime_error("writeFile failed: invalid file handle");
         }
     }},
    {0xA3, [](VMExecutionData* execData) {
         // readFile
         auto handle = getInt(execData->stack.back()); execData->stack.pop_back();
         auto varIndex = getInt(execData->stack.back()); execData->stack.pop_back();

         auto it = fileHandles.find(handle);
         if(it != fileHandles.end()) {
             auto f = it->second;
             std::string contents((std::istreambuf_iterator<char>(*f)), std::istreambuf_iterator<char>());

             execData->variables[varIndex].type = TAG_STRING;
             execData->variables[varIndex].data = contents;
         } else {
             throw std::runtime_error("readFile failed: invalid file handle");
         }
     }},
    {0xA4, [](VMExecutionData* execData) {
         // closeFile
         auto handle = getInt(execData->stack.back()); execData->stack.pop_back();

         auto it = fileHandles.find(handle);
         if(it != fileHandles.end()) {
             auto f = it->second;
             f->close();
             fileHandles.erase(it);
         } else {
             throw std::runtime_error("writeFile failed: invalid file handle");
         }
     }},
    {0xA5, [](VMExecutionData* execData) {
         // randomSeed
         auto seed = getInt(execData->stack.back()); execData->stack.pop_back();

         rngEngine.seed(seed);
     }},
    {0xA6, [](VMExecutionData* execData) {
         // random
         auto varIndex = getInt(execData->stack.back()); execData->stack.pop_back();

         static std::uniform_real_distribution<double> dist(0.0, 1.0);
         float val = dist(rngEngine);

         execData->variables[varIndex].type = TAG_FLOAT;
         execData->variables[varIndex].data = val;
     }},
    {0xA7, [](VMExecutionData* execData) {
         // randomRange
         auto varIndex = getInt(execData->stack.back()); execData->stack.pop_back();
         auto max = getInt(execData->stack.back()); execData->stack.pop_back();
         auto min = getInt(execData->stack.back()); execData->stack.pop_back();

         std::uniform_int_distribution<int64_t> dist(min, max); // inclusive on both ends
         int64_t val = dist(rngEngine);

         execData->variables[varIndex].type = TAG_INT;
         execData->variables[varIndex].data = val;
     }},
    {0xA8, [](VMExecutionData* execData) {
         // httpRequest
         auto outVarIndex = getInt(execData->stack.back()); execData->stack.pop_back();
         auto statusVarIndex = getInt(execData->stack.back()); execData->stack.pop_back();
         auto body = std::get<std::string>(execData->stack.back().data); execData->stack.pop_back();
         auto headerStr = std::get<std::string>(execData->stack.back().data); execData->stack.pop_back();
         auto url = std::get<std::string>(execData->stack.back().data); execData->stack.pop_back();
         auto method = std::get<std::string>(execData->stack.back().data); execData->stack.pop_back();

         int outStatus;
         std::string outResponse;

         std::string host, path;
         if (!splitUrl(url, host, path)) {
             throw std::runtime_error("httpGet failed: invalid url");
         }

         httplib::Client cli(host);
         cli.set_connection_timeout(5, 0);
         cli.set_read_timeout(10, 0);
         cli.set_follow_location(true);

         httplib::Headers headers = parseHeaders(headerStr);
         std::string m = toUpper(method);

         httplib::Result res;
         if (m == "GET") {
             res = cli.Get(path, headers);
         } else if (m == "POST") {
             res = cli.Post(path, headers, body, "application/octet-stream");
         } else if (m == "PUT") {
             res = cli.Put(path, headers, body, "application/octet-stream");
         } else if (m == "DELETE") {
             res = cli.Delete(path, headers);
         } else {
             throw std::runtime_error("unsupported method: " + method);
         }

         if (res) {
             outStatus = res->status;
             outResponse = res->body;
         } else {
             outStatus = -1;
             outResponse = "request failed: " + httplib::to_string(res.error());
         }

         execData->variables[outVarIndex].type = TAG_STRING;
         execData->variables[outVarIndex].data = outResponse;

         execData->variables[statusVarIndex].type = TAG_INT;
         execData->variables[statusVarIndex].data = outStatus;
     }},
    {0xA9, [](VMExecutionData* execData) {
         // strlen
         auto varIndex = getInt(execData->stack.back()); execData->stack.pop_back();
         auto value = execData->stack.back(); execData->stack.pop_back();

         auto str = std::get<std::string>(value.data);

         execData->variables[varIndex].type = TAG_INT;
         execData->variables[varIndex].data = static_cast<int64_t>(str.size());
     }},
    {0xAA, [](VMExecutionData* execData) {
         // substr(s, start, len, &out)
         auto varIndex = getInt(execData->stack.back()); execData->stack.pop_back();
         auto lenArg = getInt(execData->stack.back()); execData->stack.pop_back();
         auto startArg = getInt(execData->stack.back()); execData->stack.pop_back();
         auto value = execData->stack.back(); execData->stack.pop_back();

         auto str = std::get<std::string>(value.data);

         std::string result;
         if (startArg >= 0 && static_cast<size_t>(startArg) < str.size()) {
             result = str.substr(startArg, lenArg);
         }

         execData->variables[varIndex].type = TAG_STRING;
         execData->variables[varIndex].data = result;
     }},
    {0xAB, [](VMExecutionData* execData) {
         // strfind(s, needle, &index)
         auto varIndex = getInt(execData->stack.back()); execData->stack.pop_back();
         auto needleVal = execData->stack.back(); execData->stack.pop_back();
         auto strVal = execData->stack.back(); execData->stack.pop_back();

         auto str = std::get<std::string>(strVal.data);
         auto needle = std::get<std::string>(needleVal.data);

         auto pos = str.find(needle);
         int64_t result = (pos == std::string::npos) ? -1 : static_cast<int64_t>(pos);

         execData->variables[varIndex].type = TAG_INT;
         execData->variables[varIndex].data = result;
     }},
    {0xAC, [](VMExecutionData* execData) {
         // toUpper(s, &out) / toLower(s, &out) via a flag arg (0=lower, 1=upper)
         auto varIndex = getInt(execData->stack.back()); execData->stack.pop_back();
         auto upperFlag = getInt(execData->stack.back()); execData->stack.pop_back();
         auto value = execData->stack.back(); execData->stack.pop_back();

         auto str = std::get<std::string>(value.data);

         if (upperFlag) {
             std::transform(str.begin(), str.end(), str.begin(), ::toupper);
         } else {
             std::transform(str.begin(), str.end(), str.begin(), ::tolower);
         }

         execData->variables[varIndex].type = TAG_STRING;
         execData->variables[varIndex].data = str;
     }},
    {0xAD, [](VMExecutionData* execData) {
         // trim(s, &out)
         auto varIndex = getInt(execData->stack.back()); execData->stack.pop_back();
         auto value = execData->stack.back(); execData->stack.pop_back();

         auto str = std::get<std::string>(value.data);

         const char* ws = " \t\n\r\f\v";
         size_t start = str.find_first_not_of(ws);
         size_t end = str.find_last_not_of(ws);

         std::string result = (start == std::string::npos) ? "" : str.substr(start, end - start + 1);

         execData->variables[varIndex].type = TAG_STRING;
         execData->variables[varIndex].data = result;
     }},
    {0xD0, [](VMExecutionData* execData) {
        // setCell
        auto value = execData->stack.back(); execData->stack.pop_back();
        auto col = getInt(execData->stack.back()) - 1; execData->stack.pop_back();
        auto row = getInt(execData->stack.back()) - 1; execData->stack.pop_back();

        auto model = Widget::instance->getTableModel();

        switch(value.type) {
        case TypeTag::TAG_STRING:
        {
            auto cellValue = std::get<std::string>(value.data);
            model->setData(model->index(row, col), QString::fromStdString(cellValue), Qt::EditRole);
            break;
        }
        case TypeTag::TAG_INT:
        {
            auto cellValue = std::get<int64_t>(value.data);
            model->setData(model->index(row, col), QString::number(cellValue), Qt::EditRole);
            break;
        }
        }
    }},
    {0xD1, [](VMExecutionData* execData) {
        // setCell
        auto varIndex = getInt(execData->stack.back()); execData->stack.pop_back();
        auto col = getInt(execData->stack.back()) - 1; execData->stack.pop_back();
        auto row = getInt(execData->stack.back()) - 1; execData->stack.pop_back();

        auto model = Widget::instance->getTableModel();

        execData->variables[varIndex].type = TAG_STRING;
        execData->variables[varIndex].data = model->getCell(row, col)->value.toString().toStdString();
     }}
};
