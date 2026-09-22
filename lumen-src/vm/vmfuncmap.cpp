#include "lumen-inc/vm.h"
#include "lumen-inc/helpers.h"

#include "widget.h"
#include "scriptingpanel.h"
#include <condition_variable>
#include <QThread>
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
    // -------------------------------------------------------------------------
    // Qt-based Console I/O
    // -------------------------------------------------------------------------
    {0x01, [](VMExecutionData* execData) {
         // println (Qt output)
         auto& stack = execData->stack;
         auto arg0 = stack.back(); stack.pop_back();
         std::visit([](const auto& val) {
             std::ostringstream oss;
             oss << val;
             ScriptingPanel::appendOutput(QString::fromStdString(oss.str()));
         }, arg0.data);
         ScriptingPanel::appendOutput("\n");
     }},
    {0x02, [](VMExecutionData* execData) {
         // print (Qt output)
         auto& stack = execData->stack;
         auto arg0 = stack.back(); stack.pop_back();
         std::visit([](const auto& val) {
             std::ostringstream oss;
             oss << val;
             ScriptingPanel::appendOutput(QString::fromStdString(oss.str()));
         }, arg0.data);
     }},
    {0x03, [](VMExecutionData* execData) {
         // inputInt (Qt InputDialog -> Stack Push)
         auto& stack = execData->stack;

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

             cv.wait(lock, [&] { return done; });
         }

         stack.push_back({TAG_INT, static_cast<int64_t>(result)});
     }},
    {0x04, [](VMExecutionData* execData) {
         // inputStr (Qt InputDialog -> Stack Push)
         auto& stack = execData->stack;

         std::mutex mtx;
         std::condition_variable cv;
         bool done = false;
         QString result;

         {
             std::unique_lock lock(mtx);

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

             cv.wait(lock, [&] { return done; });
         }

         stack.push_back({TAG_STRING, result.toStdString()});
     }},

    // -------------------------------------------------------------------------
    // Standard Type Conversions
    // -------------------------------------------------------------------------
    {0x05, [](VMExecutionData* execData) {
         // str2int
         auto& stack = execData->stack;
         auto value = stack.back(); stack.pop_back();

         int num = 0;
         std::string str = "0";
         if (value.type == TAG_STRING) {
             str = std::get<std::string>(value.data);
         }

         try {
             num = std::stoi(str);
         } catch (...) {
             num = 0;
         }

         stack.push_back({TAG_INT, static_cast<int64_t>(num)});
     }},
    {0x06, [](VMExecutionData* execData) {
         // int2str
         auto& stack = execData->stack;
         auto value = stack.back(); stack.pop_back();

         int num = getInt(value);
         std::string str = std::to_string(num);
         stack.push_back({TAG_STRING, str});
     }},
    {0x07, [](VMExecutionData* execData) {
         // str2float
         auto& stack = execData->stack;
         auto value = stack.back(); stack.pop_back();

         double num = 0.0;
         std::string str = "0";
         if (value.type == TAG_STRING) {
             str = std::get<std::string>(value.data);
         }

         try {
             num = std::stod(str);
         } catch (...) {
             num = 0.0;
         }

         stack.push_back({TAG_FLOAT, num});
     }},
    {0x08, [](VMExecutionData* execData) {
         // float2str
         auto& stack = execData->stack;
         auto value = stack.back(); stack.pop_back();

         double num = 0.0;
         if (value.type == TAG_FLOAT) num = std::get<double>(value.data);
         else if (value.type == TAG_INT) num = static_cast<double>(getInt(value));

         std::string str = std::to_string(num);
         stack.push_back({TAG_STRING, str});
     }},

    // -------------------------------------------------------------------------
    // Standard Utility & System Opcodes
    // -------------------------------------------------------------------------
    {0xA0, [](VMExecutionData* execData) {
         auto& stack = execData->stack;
         auto value = stack.back(); stack.pop_back();
         if (value.type != TAG_STRING) {
             throw std::runtime_error("assertCapability failed: invalid value type");
         }
         auto str = std::get<std::string>(value.data);
         auto it = capabilitySet.find(str);
         if (it == capabilitySet.end()) {
             std::stringstream ss;
             ss << "assertCapability failed: capability " << str << " is not present";
             throw std::runtime_error(ss.str());
         }
     }},
    {0xA1, [](VMExecutionData* execData) {
         auto& stack = execData->stack;
         auto value = stack.back(); stack.pop_back();

         auto filename = std::get<std::string>(value.data);

         auto stream = new std::fstream(filename, std::ios::in | std::ios::out | std::ios::app);
         if (!stream->is_open()) {
             delete stream;
             throw std::runtime_error("openFile failed: unable to open file " + filename);
         }

         fileHandles[fileHandleId] = stream;
         stack.push_back({TAG_INT, static_cast<int64_t>(fileHandleId++)});
     }},
    {0xA2, [](VMExecutionData* execData) {
         auto& stack = execData->stack;
         auto handle = getInt(stack.back()); stack.pop_back();
         auto value = stack.back(); stack.pop_back();

         auto valueToWrite = std::get<std::string>(value.data);

         auto it = fileHandles.find(handle);
         if (it != fileHandles.end()) {
             auto f = it->second;
             *f << valueToWrite;
             f->flush();
         } else {
             throw std::runtime_error("writeFile failed: invalid file handle");
         }
     }},
    {0xA3, [](VMExecutionData* execData) {
         auto& stack = execData->stack;
         auto handle = getInt(stack.back()); stack.pop_back();

         auto it = fileHandles.find(handle);
         if (it != fileHandles.end()) {
             auto f = it->second;
             f->flush();
             f->clear();
             f->seekg(0, std::ios::beg);
             std::string contents((std::istreambuf_iterator<char>(*f)), std::istreambuf_iterator<char>());
             f->clear();

             stack.push_back({TAG_STRING, contents});
         } else {
             throw std::runtime_error("readFile failed: invalid file handle");
         }
     }},
    {0xA4, [](VMExecutionData* execData) {
         auto& stack = execData->stack;
         auto handle = getInt(stack.back()); stack.pop_back();

         auto it = fileHandles.find(handle);
         if (it != fileHandles.end()) {
             auto f = it->second;
             f->close();
             delete f;
             fileHandles.erase(it);
         } else {
             throw std::runtime_error("closeFile failed: invalid file handle");
         }
     }},
    {0xA5, [](VMExecutionData* execData) {
         auto& stack = execData->stack;
         auto seed = getInt(stack.back()); stack.pop_back();

         rngEngine.seed(seed);
     }},
    {0xA6, [](VMExecutionData* execData) {
         auto& stack = execData->stack;

         static std::uniform_real_distribution<double> dist(0.0, 1.0);
         double val = dist(rngEngine);

         stack.push_back({TAG_FLOAT, val});
     }},
    {0xA7, [](VMExecutionData* execData) {
         auto& stack = execData->stack;
         auto max = getInt(stack.back()); stack.pop_back();
         auto min = getInt(stack.back()); stack.pop_back();

         std::uniform_int_distribution<int64_t> dist(min, max);
         int64_t val = dist(rngEngine);

         stack.push_back({TAG_INT, val});
     }},
    {0xA8, [](VMExecutionData* execData) {
         auto& stack = execData->stack;
         auto bodyOutVarIndex = getInt(stack.back()); stack.pop_back();
         auto body = std::get<std::string>(stack.back().data); stack.pop_back();
         auto headerStr = std::get<std::string>(stack.back().data); stack.pop_back();
         auto url = std::get<std::string>(stack.back().data); stack.pop_back();
         auto method = std::get<std::string>(stack.back().data); stack.pop_back();

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

         std::string contentType = "application/octet-stream";
         for (auto it = headers.begin(); it != headers.end(); ) {
             if (toUpper(it->first) == "CONTENT-TYPE") {
                 contentType = it->second;
                 it = headers.erase(it);
             } else {
                 ++it;
             }
         }

         httplib::Result res;
         if (m == "GET") {
             res = cli.Get(path, headers);
         } else if (m == "POST") {
             res = cli.Post(path, headers, body, contentType);
         } else if (m == "PUT") {
             res = cli.Put(path, headers, body, contentType);
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

         writeVariable(execData, bodyOutVarIndex, TAG_STRING, outResponse);
         stack.push_back({TAG_INT, static_cast<int64_t>(outStatus)});
     }},
    {0xA9, [](VMExecutionData* execData) {
         auto& stack = execData->stack;
         auto value = stack.back(); stack.pop_back();

         auto str = std::get<std::string>(value.data);

         stack.push_back({TAG_INT, static_cast<int64_t>(str.size())});
     }},
    {0xAA, [](VMExecutionData* execData) {
         auto& stack = execData->stack;
         auto lenArg = getInt(stack.back()); stack.pop_back();
         auto startArg = getInt(stack.back()); stack.pop_back();
         auto value = stack.back(); stack.pop_back();

         auto str = std::get<std::string>(value.data);

         std::string result;
         if (startArg >= 0 && static_cast<size_t>(startArg) < str.size()) {
             result = str.substr(startArg, lenArg);
         }

         stack.push_back({TAG_STRING, result});
     }},
    {0xAB, [](VMExecutionData* execData) {
         auto& stack = execData->stack;
         auto needleVal = stack.back(); stack.pop_back();
         auto strVal = stack.back(); stack.pop_back();

         auto str = std::get<std::string>(strVal.data);
         auto needle = std::get<std::string>(needleVal.data);

         auto pos = str.find(needle);
         int64_t result = (pos == std::string::npos) ? -1 : static_cast<int64_t>(pos);

         stack.push_back({TAG_INT, result});
     }},
    {0xAC, [](VMExecutionData* execData) {
         auto& stack = execData->stack;
         auto upperFlag = getInt(stack.back()); stack.pop_back();
         auto value = stack.back(); stack.pop_back();

         auto str = std::get<std::string>(value.data);

         if (upperFlag) {
             std::transform(str.begin(), str.end(), str.begin(), ::toupper);
         } else {
             std::transform(str.begin(), str.end(), str.begin(), ::tolower);
         }

         stack.push_back({TAG_STRING, str});
     }},
    {0xAD, [](VMExecutionData* execData) {
         auto& stack = execData->stack;
         auto value = stack.back(); stack.pop_back();

         auto str = std::get<std::string>(value.data);

         const char* ws = " \t\n\r\f\v";
         size_t start = str.find_first_not_of(ws);
         size_t end = str.find_last_not_of(ws);

         std::string result = (start == std::string::npos) ? "" : str.substr(start, end - start + 1);

         stack.push_back({TAG_STRING, result});
     }},

    // -------------------------------------------------------------------------
    // Spreadsheet-Specific Opcodes (Rewritten to New Standard)
    // -------------------------------------------------------------------------
    {0xD0, [](VMExecutionData* execData) {
         // setCell(row, col, value)
         auto& stack = execData->stack;
         auto value = stack.back(); stack.pop_back();
         auto col = getInt(stack.back()) - 1; stack.pop_back();
         auto row = getInt(stack.back()) - 1; stack.pop_back();

         auto model = Widget::instance->getTableModel();

         switch (value.type) {
         case TypeTag::TAG_STRING: {
             auto cellValue = std::get<std::string>(value.data);
             model->setData(model->index(row, col), QString::fromStdString(cellValue), Qt::EditRole);
             break;
         }
         case TypeTag::TAG_INT: {
             auto cellValue = std::get<int64_t>(value.data);
             model->setData(model->index(row, col), QString::number(cellValue), Qt::EditRole);
             break;
         }
         case TypeTag::TAG_FLOAT: {
             auto cellValue = std::get<double>(value.data);
             model->setData(model->index(row, col), QString::number(cellValue), Qt::EditRole);
             break;
         }
         default:
             break;
         }
     }},
    {0xD1, [](VMExecutionData* execData) {
         // getCell(row, col) -> pushes cell string to stack
         auto& stack = execData->stack;
         auto col = getInt(stack.back()) - 1; stack.pop_back();
         auto row = getInt(stack.back()) - 1; stack.pop_back();

         auto model = Widget::instance->getTableModel();
         std::string cellValue = model->getCell(row, col)->value.toString().toStdString();

         stack.push_back({TAG_STRING, cellValue});
     }}
};