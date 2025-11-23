#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cmath>
#include <ctime>

using namespace std;

struct Task {
    string name;
    int deadline;  // 剩余天数（非负数）
};

const string TASKS_FILE_CONFIG = "tasks_file_path.txt"; // 用于保存用户选择的路径的配置文件
const string DATE_FILE = "date.txt";

// 获取当前日期 (YYYY-MM-DD)
string getCurrentDate() {
    time_t t = time(nullptr);
    tm* now = localtime(&t);
    char buffer[11];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", now);
    return string(buffer);
}

// 计算两个日期的间隔天数
int daysBetween(const string& oldDate, const string& newDate) {
    tm oldTm = {}, newTm = {};
    sscanf(oldDate.c_str(), "%d-%d-%d", &oldTm.tm_year, &oldTm.tm_mon, &oldTm.tm_mday);
    sscanf(newDate.c_str(), "%d-%d-%d", &newTm.tm_year, &newTm.tm_mon, &newTm.tm_mday);
    oldTm.tm_year -= 1900; oldTm.tm_mon -= 1;
    newTm.tm_year -= 1900; newTm.tm_mon -= 1;
    time_t oldTime = mktime(&oldTm);
    time_t newTime = mktime(&newTm);
    return (newTime - oldTime) / (60 * 60 * 24);
}

// 读取上次运行日期
string loadLastDate() {
    ifstream file(DATE_FILE);
    string lastDate;
    if (file) {
        getline(file, lastDate);
        file.close();
    }
    return lastDate.empty() ? getCurrentDate() : lastDate;
}

// 更新日期文件
void saveCurrentDate(const string& currentDate) {
    ofstream file(DATE_FILE);
    file << currentDate;
    file.close();
}

// 读取任务文件路径配置
string loadTasksFilePath() {
    ifstream file(TASKS_FILE_CONFIG);
    string path;
    if (file) {
        getline(file, path);
        file.close();
    }
    return path;
}

// 保存任务文件路径配置
void saveTasksFilePath(const string& path) {
    ofstream file(TASKS_FILE_CONFIG);
    file << path;
    file.close();
}

// 从文件加载任务
vector<Task> loadTasks(const string& taskFilePath) {
    vector<Task> tasks;
    ifstream file(taskFilePath);
    if (!file) {
        cout << "未找到任务文件，将创建新任务列表。\n";
        return tasks;
    }
    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        Task task;
        ss >> task.name >> task.deadline;
        tasks.push_back(task);
    }
    file.close();
    return tasks;
}

// 保存任务到文件
void saveTasks(const vector<Task>& tasks, const string& taskFilePath) {
    ofstream file(taskFilePath);
    for (const auto& task : tasks) {
        file << task.name << " " << task.deadline << "\n";
    }
    file.close();
}

// 自动更新任务的 ddl（根据日期差值，每天减少对应天数）
// 自动更新任务的 ddl（根据日期差值，每天减少对应天数）
void updateDeadlines(vector<Task>& tasks) {
    string lastDate = loadLastDate();
    string currentDate = getCurrentDate();

    int daysPassed = daysBetween(lastDate, currentDate);
    if (daysPassed <= 0) {
        cout << "日期未变化或回退，不更新任务DDL。\n";
        return;
    }

    cout << "已过去 " << daysPassed << " 天，正在更新任务DDL...\n";
    for (auto& task : tasks) {
        task.deadline = max(0, task.deadline - daysPassed);  // 确保不会变负数
        cout << "任务: " << task.name << " 新DDL: " << task.deadline << " 天\n";
    }

    saveTasks(tasks, loadTasksFilePath());
    saveCurrentDate(currentDate);
}


// 使用 logistic 函数计算任务数量评分
double computeTasksScore(int n) {
    // 参数设置：a 控制下降速度，n0 为拐点（最佳任务数 ≤ 5）
    double a = 0.8, n0 = 5.0;
    double score = 100.0 / (1.0 + exp(a * (n - n0)));
    return score;
}

// 使用 logistic 函数计算截止日期评分（以最紧迫任务为基准）
double computeDeadlineScore(const vector<Task>& tasks) {
    if(tasks.empty()) return 100.0;
    int minDeadline = 10000;
    for (const auto& task : tasks) {
        if (task.deadline < minDeadline)
            minDeadline = task.deadline;
    }
    // 参数设置：b 控制曲线陡峭度，d0 为拐点（最好时 d >= 2）
    double b = 1.5, d0 = 2.0;
    double score = 100.0 / (1.0 + exp(-b * (minDeadline - d0)));
    return score;
}

// 综合评分：采用加权几何平均，ddl 因子权重更大（例如70%）
double evaluateCompletion(const vector<Task>& tasks) {
    int n = tasks.size();
    double tasksScore = computeTasksScore(n);
    double deadlineScore = computeDeadlineScore(tasks);

    // 设定截止日期权重为 0.7，任务数量权重为 0.3
    double w = 0.7;
    // 加权几何平均（取对数再加权求指数）
    double overallScore = exp(w * log(deadlineScore) + (1 - w) * log(tasksScore));
    return overallScore;
}

// 显示当前评分及状态提示
void showScore(const vector<Task>& tasks) {
    double score = evaluateCompletion(tasks);
    cout << "\n当前作业完成度得分: " << score << endl;
    if (score >= 80) {
        cout << "状态非常好，可以躺平！\n";
    } else if (score >= 60) {
        cout << "状态尚可，但建议尽快处理任务！（及格线）\n";
    } else {
        cout << "状态不理想，请立即处理任务！（低于及格线）\n";
    }
}

// 显示任务列表
void listTasks(const vector<Task>& tasks) {
    if (tasks.empty()) {
        cout << "当前没有任务。\n";
        return;
    }
    cout << "\n当前任务列表：\n";
    for (const auto& task : tasks) {
        cout << "任务: " << task.name << "，剩余 " << task.deadline << " 天\n";
    }
}

// 添加新任务
void addTask(vector<Task>& tasks) {
    Task task;
    cout << "请输入任务名称: ";
    cin >> task.name;
    cout << "请输入任务截止日期（距离今天的天数）: ";
    cin >> task.deadline;
    tasks.push_back(task);
    saveTasks(tasks, loadTasksFilePath());
    cout << "任务已添加并保存。\n";
}

// 删除任务（根据任务名称完全匹配）
void deleteTask(vector<Task>& tasks, const string& taskName) {
    auto it = remove_if(tasks.begin(), tasks.end(), [&](const Task& t) {
        return t.name == taskName;
    });
    if (it != tasks.end()) {
        tasks.erase(it, tasks.end());
        saveTasks(tasks, loadTasksFilePath());
        cout << "任务 " << taskName << " 已删除。\n";
    } else {
        cout << "未找到任务 " << taskName << "。\n";
    }
}

int main() {
    vector<Task> tasks = loadTasks(loadTasksFilePath()); // 加载任务
    updateDeadlines(tasks); // 立即更新所有任务的 ddl
    string currentDate = getCurrentDate();
    saveCurrentDate(currentDate);
    string taskFilePath = loadTasksFilePath(); // 读取文件路径
    if (taskFilePath.empty()) {
        // 如果没有路径，则是第一次运行，提示用户选择路径
        cout << "这是第一次运行，请选择一个文件保存路径（例如：/Users/your_username/Documents/tasks.txt）：\n";
        getline(cin, taskFilePath);
        saveTasksFilePath(taskFilePath); // 保存用户选择的路径
    }

    // 初始显示评分
    showScore(tasks);

    // 用户交互界面
    cout << "\n欢迎使用任务管理系统，输入以下命令进行操作：\n"
         << "  list            - 显示所有任务\n"
         << "  add             - 添加新任务\n"
         << "  <任务名称> done - 删除指定任务（例如: task1 done）\n"
         << "  score           - 显示当前评分\n"
         << "  exit            - 退出程序\n";

    string command;
    cin.ignore();
    while (true) {
        cout << "\n请输入命令：";
        getline(cin, command);
        if (command == "list") {
            listTasks(tasks);
        } else if (command == "add") {
            addTask(tasks);
        } else if (command == "score") {
            showScore(tasks);
        } else if (command == "exit") {
            break;
        } else {
            stringstream ss(command);
            string taskName, action;
            ss >> taskName >> action;
            if (action == "done") {
                deleteTask(tasks, taskName);
            } else {
                cout << "未知命令，请重新输入。\n";
            }
        }
    }

    return 0;
}