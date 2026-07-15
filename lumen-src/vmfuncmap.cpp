#include "lumen-inc/vm.h"

#include "widget.h"
#include <condition_variable>
#include <QThread> // TODO:  temp
#include "scriptingpanel.h"
#include <QInputDialog>

std::unordered_map<int, NativeFn> funcMap = {
    {0x01, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        // println
        auto arg0 = stack.back(); stack.pop_back();
        std::visit([](const auto& val) {
            std::ostringstream oss;
            oss << val;
            ScriptingPanel::appendOutput(QString::fromStdString(oss.str()));
        }, arg0.data);
        ScriptingPanel::appendOutput("\n");
    }},
    {0x02, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        // print
        auto arg0 = stack.back(); stack.pop_back();
        std::visit([](const auto& val) {
            std::ostringstream oss;
            oss << val;
            ScriptingPanel::appendOutput(QString::fromStdString(oss.str()));
        }, arg0.data);
    }},
    {0x03, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
         // inputInt
         auto varIndex = getInt(stack.back());
         stack.pop_back();

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

         variables[varIndex].type = TAG_INT;
         variables[varIndex].data = static_cast<int64_t>(result);
     }},
    {0x04, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        // inputStr
        auto varIndex = getInt(stack.back()); stack.pop_back();

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

        variables[varIndex].type = TAG_STRING;
        variables[varIndex].data = result.toStdString();
    }},
    {0x05, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
         // str2int
         auto varIndex = getInt(stack.back()); stack.pop_back();
         auto value = stack.back(); stack.pop_back();

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

         variables[varIndex].type = TAG_INT;
         variables[varIndex].data = num;
     }},
    {0x06, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
         // int2str
         auto varIndex = getInt(stack.back()); stack.pop_back();
         auto value = stack.back(); stack.pop_back();

         int num = 0;
         if(value.type == TAG_INT) num = getInt(value);

         std::string str = std::to_string(num);
         variables[varIndex].type = TAG_STRING;
         variables[varIndex].data = str;
     }},
    {0xAA00, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        // setCell
        auto value = stack.back(); stack.pop_back();
        auto col = getInt(stack.back()) - 1; stack.pop_back();
        auto row = getInt(stack.back()) - 1; stack.pop_back();

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
    {0xAA01, [](std::vector<Variant>& stack, std::vector<Variant>& variables) {
        // setCell
        auto varIndex = getInt(stack.back()); stack.pop_back();
        auto col = getInt(stack.back()) - 1; stack.pop_back();
        auto row = getInt(stack.back()) - 1; stack.pop_back();

        auto model = Widget::instance->getTableModel();

        variables[varIndex].type = TAG_STRING;
        variables[varIndex].data = model->getCell(row, col)->value.toString().toStdString();
     }}
};
