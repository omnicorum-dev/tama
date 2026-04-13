#include <filesystem>
#include <__filesystem/filesystem_error.h>

#define DISABLE_TRACE
#include "base.h"
#include <fstream>

using namespace omni;
using std::string;
namespace fs = std::filesystem;

struct task {
    u64 uid;
    string name;
    bool isOpen;
    std::vector<string> tags;
    i32 priority;

    string description;

    friend std::ostream& operator<<(std::ostream& os, const task& t) {
        os << t.name << " - " << t.uid << "\n";
        if (t.isOpen) {
            os << "OPEN" << "\n";
        } else {
            os << "CLOSED" << "\n";
        }
        os << "priority: " << t.priority << "\n";

        os << "tags: \n";
        for (const string& tag : t.tags) {
            os << "    " << tag << "\n";
        }
        return os;
    }

    bool has_tag(const string& tag) {
        return std::find(tags.begin(), tags.end(), tag) != tags.end();
    }
};

template<typename T>
string vectorToString (std::vector<T> vec) {
    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); i++) {
        oss << vec[i];
        if (i + 1 != vec.size()) oss << ", ";
    }
    return oss.str();
}

class task_calculator {
public:
    const std::array<string, 10> OPERATORS = {"(", ")", "impl", "nor", "or", "xnor", "xor", "nand", "and", "not"};
    const std::array<int, 10> PRECEDENCE = { -1, -1, 1, 2, 2, 3, 3, 4, 4, 5 };

    bool pushedOp = false;
    std::vector<string> operatorStack;
    std::vector<bool> boolStack;

    void clearStacks() {
        operatorStack.clear();
        boolStack.clear();
    }

    void displayStacks() const {
        LOG_TRACE("operator stack: {}", vectorToString(operatorStack));
        LOG_TRACE("bool stack:     {}\n", vectorToString(boolStack));
    }

    void doBool (bool x) {
        boolStack.push_back(x);
        pushedOp = false;
        displayStacks();
    }

    void doOperator(string op) {
        if ((operatorStack.empty() && boolStack.empty()) || pushedOp) {
            op = "not";
        }
        processOperator(op);
        pushedOp = true;
        displayStacks();
    }

    bool doEquals() {
        while (!operatorStack.empty()) {
            evaluateTopOperator();
        }

        if (boolStack.empty()) {
            return true;
        }
        return boolStack.back();
    }

    bool evaluateOperator (const bool a, const string& op, const bool b) {
        if (op == "and") {
            return a && b;
        } else if (op == "or") {
            return a || b;
        } else if (op == "xor") {
            return (a && !b) || (!a && b);
        } else if (op == "nand") {
            return !(a && b);
        } else if (op == "nor") {
            return !(a || b);
        } else if (op == "xnor") {
            return (a && b) || (!a && !b);
        } else if (op == "impl") {
            return !a || b;
        } else {
            LOG_WARN("Unknown operator '{}'", op);
            return false;
        }
    }

    void evaluateTopOperator() {
        string op = operatorStack.back();
        operatorStack.pop_back();
        if (op == "not") {
            bool a = boolStack.back();
            boolStack.pop_back();
            a = !a;
            boolStack.push_back(a);
        } else {
            bool b = boolStack.back();
            boolStack.pop_back();
            bool a = boolStack.back();
            boolStack.pop_back();

            bool res = evaluateOperator(a, op, b);

            boolStack.push_back(res);
        }

        displayStacks();
    }

    int precedence(const string& op) {
        for (size_t i = 0; i < OPERATORS.size(); i++) {
            if (OPERATORS[i] == op) {
                return PRECEDENCE[i];
            }
        }
        throw std::runtime_error("Unknown operator: " + op);
    }

    void processOperator(const string& op) {
        if (op == "(" || op == "not") {
            operatorStack.push_back(op);
            return;
        }

        if (op == ")") {
            while (!operatorStack.empty() &&
                operatorStack.back() != "(" &&
                precedence(operatorStack.back()) >= precedence(op)) {
                evaluateTopOperator();
            }
            operatorStack.pop_back();
            return;
        }

        string top = !operatorStack.empty() ? operatorStack.back() : "";
        if (top != "" && precedence(top) >= precedence(op)) {
            evaluateTopOperator();
        }

        operatorStack.push_back(op);
    }

    bool processExpression(task& t, const char* expr[], int start_idx, int argc) {
        for (int i = start_idx; i < argc; i++) {
            string arg = expr[i];
            bool isOperator = std::find(OPERATORS.begin(), OPERATORS.end(), arg) != OPERATORS.end();
            if (isOperator) {
                doOperator(arg);
            } else {
                // is either a tag or open/closed
                if (arg == "open") {
                    doBool(t.isOpen);
                } else if (arg == "closed") {
                    doBool(!t.isOpen);
                } else {
                    doBool(t.has_tag(arg));
                }
            }
        }
        bool res = doEquals();
        return res;
    }
};

task make_task(const fs::path& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        LOG_ERROR("Error opening the task file!");
        return {0};
    }

    task t = {0};
    string s;
    getline(f, s);
    try {
        t.uid = std::stoull(s);
    } catch (const std::exception& e) {
        LOG_ERROR("Invalid number: {}", e.what());
    }

    getline(f, t.name);

    getline(f, s);
    try {
        t.priority = std::stoi(s);
    } catch (const std::exception& e) {
        LOG_ERROR("Invalid number: {}", e.what());
    }

    getline(f, s);
    if (s == "open") {
        t.isOpen = true;
    } else {
        t.isOpen = false;
    }

    getline(f, s);
    size_t start = 0;
    while (true) {
        size_t end = s.find(',', start);
        t.tags.emplace_back(s.substr(start, end - start));
        if (end == std::string::npos) break;
        start = end + 1;
    }

    std::ostringstream oss;
    oss << f.rdbuf();
    t.description = oss.str();

    f.close();

    return t;
}

fs::path find_tasks_directory(fs::path p) {
    for (int i = 0; i < 4; i++) {
        fs::path candidate = p / "TASKS";
        if (fs::exists(candidate)) return candidate;
        p = p.parent_path();
    }
    return {};
}

fs::path find_task(const fs::path& tasksPath, u64 uid) {
    return tasksPath / std::to_string(uid) / "task.txt";

    string uid_string = std::to_string(uid);

    fs::path taskPath = tasksPath / uid_string;
    if (fs::exists(taskPath)) {
        fs::path task = taskPath / "task.txt";
        if (fs::exists(task)) {
            return task;
        }
    }
    return {};
}

void list_tasks(fs::path tasksPath, const char* expr[], int argc) {
    std::vector<task> tasks;

    for (auto& entry : fs::directory_iterator(tasksPath)) {
        fs::path taskFile = entry.path() / "task.txt";
        if (!fs::exists(taskFile)) continue;

        tasks.push_back(make_task(taskFile));
    }

    std::sort(tasks.begin(), tasks.end(),
        [](const task& a, const task& b) {
            return a.priority > b.priority; // or >
        });

    task_calculator calculator;

    for (auto& t : tasks) {
        calculator.clearStacks();

        bool res = calculator.processExpression(t, expr, 2, argc);

        if (res) {
            println("{}", t);
        }
    }
}

void create_tasks_directory() {
    const fs::path tasksPath = fs::current_path() / "TASKS";
    if (fs::exists(tasksPath)) {
        LOG_WARN("TASKS folder already exists in current directory. Ignoring.");
        return;
    }

    try {
        fs::create_directory(tasksPath);
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("Error creating tasks folder: {}", e.what());
    }
}

void create_task_file(const task& t) {
    fs::path tasksPath = find_tasks_directory(fs::current_path());

    fs::path taskPath = tasksPath / std::to_string(t.uid);
    try {
        fs::create_directory(taskPath);
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("Error creating tasks folder: {}", e.what());
    }

    fs::path taskFile = taskPath / "task.txt";

    std::ofstream f(taskFile);
    f << t.uid << "\n" << t.name << "\n" << t.priority << "\n";
    if (t.isOpen) {
        f << "open\n";
    } else {
        f << "closed\n";
    }
    for (size_t i = 0; i < t.tags.size(); i++) {
        f << t.tags[i];
        if (i != t.tags.size() - 1) {
            f << ",";
        }
    }
    f << "\n\n";

    f << t.description << std::endl;
    f.close();
}

void create_new_task() {
    std::time_t now = time(nullptr);
    std::tm tm = *localtime(&now);

    task t;

    // Convert time to UID
    string s = "";
    {
        s += std::to_string(tm.tm_year + 1900);
        if (tm.tm_mon + 1 < 10) {
            s += "0" + std::to_string(tm.tm_mon + 1);
        } else {
            s += std::to_string(tm.tm_mon + 1);
        }
        if (tm.tm_mday < 10) {
            s += "0" + std::to_string(tm.tm_mday);
        } else {
            s += std::to_string(tm.tm_mday);
        }
        if (tm.tm_hour < 10) {
            s += "0" + std::to_string(tm.tm_hour);
        } else {
            s += std::to_string(tm.tm_hour);
        }
        if (tm.tm_min < 10) {
            s += "0" + std::to_string(tm.tm_min);
        } else {
            s += std::to_string(tm.tm_min);
        }
        if (tm.tm_sec < 10) {
            s += "0" + std::to_string(tm.tm_sec);
        } else {
            s += std::to_string(tm.tm_sec);
        }
    }

    t.uid = std::stoull(s);

    print("Task name: ");
    std::getline(std::cin, t.name);

    print("Task priority (integer): ");
    std::getline(std::cin, s);
    t.priority = std::stoi(s);

    println("Enter tags ('end' when completed)");
    while (getline(std::cin, s)) {
        if (s == "end") {
            break;
        }
        t.tags.emplace_back(s);
    }

    print("Task description: ");
    std::getline(std::cin, t.description);

    t.isOpen = true;

    println("\n{}", t);

    create_task_file(t);
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        println("Usage: ./tama [init] | [find]");
        return 1;
    }

    if (argv[1] == string("init")) {
        println("Initializing tasks folder...");
        create_tasks_directory();
    }

    else if (argv[1] == string("find")) {
        println("Finding tasks folder...");
        const fs::path tasksPath = find_tasks_directory(fs::current_path());
        if (tasksPath.empty()) {
            LOG_ERROR("Tasks folder not found.");
        } else {
            println("Tasks folder: {}", tasksPath);
        }
    }

    else if (argv[1] == string("ls")) {
        const fs::path tasksPath = find_tasks_directory(fs::current_path());
        if (tasksPath.empty()) {
            LOG_ERROR("Tasks folder not found.");
        }

        list_tasks(tasksPath, argv, argc);
    }

    else if (argv[1] == string("new")) {
        create_new_task();
    }

    else if (argv[1] == string("more")) {
        // Get UID from command arguments
        u64 value = 0;
        try {
            value = std::stoull(argv[2]);
        } catch (const std::exception& e) {
            LOG_ERROR("Invalid number: {}", e.what());
        }

        // get path of tasks folder
        const fs::path tasksPath = find_tasks_directory(fs::current_path());
        if (tasksPath.empty()) {
            LOG_ERROR("Tasks folder not found.");
        }

        // get path of specific task
        const fs::path taskPath = find_task(tasksPath, value);
        if (taskPath.empty()) {
            LOG_ERROR("Task not found.");
        }

        // load task file into task struct
        task t = make_task(taskPath);

        println("{}\n{}{}", taskPath.parent_path().string(), t, t.description);
    }

    else if (argv[1] == string("status")) {
        // Get UID from command arguments
        u64 value = 0;
        try {
            value = std::stoull(argv[2]);
        } catch (const std::exception& e) {
            LOG_ERROR("Invalid number: {}", e.what());
        }

        // get path of tasks folder
        const fs::path tasksPath = find_tasks_directory(fs::current_path());
        if (tasksPath.empty()) {
            LOG_ERROR("Tasks folder not found.");
        }

        // get path of specific task
        const fs::path taskPath = find_task(tasksPath, value);
        if (taskPath.empty()) {
            LOG_ERROR("Task not found.");
        }

        // load task file into task struct
        task t = make_task(taskPath);

        if (argv[2] == string("open")) {
            t.isOpen = true;
        } else {
            t.isOpen = false;
        }

        create_task_file(t);
    }

    return 0;
}