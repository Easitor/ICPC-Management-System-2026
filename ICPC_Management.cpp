#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <iterator>
#include <algorithm>
using namespace std;

// 题号 —— 0: ALL 1~26: A~Z
// 状态 —— 0：ALL AC: 1 WA: 2 RE: 3 TLE: 4
const int kMAXN = 1e4 + 5;

struct Problem{
    int first_pass_time = -1; // 第一次通过的时间
    int wrong_times = 0; // 这题正确前错误次数
};

struct Submission{
    int id = 0; // 提交序号
    int problem = -1; // 提交题号
    int status = 0; // 提交状态
    int time = 0; // 提交时间
};

struct Score {
    int pass_num = 0;
    int penalty = 0;
    Problem problem[30];
    vector<int> accept_time;
};

struct Team{
    string name; // 名字
    int number; // 编号
    int pass_num = 0; // 通过题数
    int penalty = 0; // 总罚时
    int rank = 0; // 上次刷新后的排名
    Problem problem[30];
    vector<int> accept_time; // 通过的时间
    Submission submission[30][5]; // 第i道题第j个状态最后一次提交

    Score visible;                 // 榜单可见成绩
    int frozen_submit[30] = {0};    // 每道题本次封榜后的提交次数 -x/y中的y
    
    // 一定是visible！
    bool operator<(const Team& other) const {
        if (visible.pass_num != other.visible.pass_num) {
            return visible.pass_num > other.visible.pass_num;
        }
        if (visible.penalty != other.visible.penalty) {
            return visible.penalty < other.visible.penalty;
        }
        for (int i = visible.pass_num - 1; i >= 0; i--) { // 升序排序
            if (visible.accept_time[i] != other.visible.accept_time[i]) {
                return visible.accept_time[i] < other.visible.accept_time[i];
            }
        }
        return name < other.name;
    }
};

int team_cnt; // 记录队伍总数
int submit_id; // 提交编号
int duration_time, problem_count; // 比赛时长
bool status; // 0：未开始 1：已开始
bool frozen; // 0：未封榜 1：已封榜
bool rank_change;
bool rank_changed[kMAXN];
string command;
map<string, int> shift;
set<Team> team;
Team teams[kMAXN];

int status_shift(string sta) {
    if (sta == "Accepted") return 1;
    if (sta == "Wrong_Answer") return 2;
    if (sta == "Runtime_Error") return 3;
    if (sta == "Time_Limit_Exceed") return 4;
    return 0; // ALL
}

string shift_status(int sta) {
    if (sta == 1) return "Accepted";
    if (sta == 2) return "Wrong_Answer";
    if (sta == 3) return "Runtime_Error";
    if (sta == 4) return "Time_Limit_Exceed";
    return "";
}

bool cmp(int x, int y) {
    return teams[x] < teams[y];
}

void Addteam();
void Start();
void Submit();
void Flush();
void Freeze();
void Scroll();
void QueryRanking();
void QuerySubmission();

int main() {
    ios::sync_with_stdio(0);
    cin.tie(0); cout.tie(0);
    // freopen(".out", "w", stdout);
    while (cin >> command) {
        if (command == "END") {
            cout << "[Info]Competition ends.\n";
            break;
        }
        if (command == "ADDTEAM") Addteam();
        if (command == "START") Start();
        if (command == "SUBMIT") Submit();
        if (command == "FLUSH") {
            cout << "[Info]Flush scoreboard.\n";
            Flush();
        }
        if (command == "FREEZE") Freeze();
        if (command == "SCROLL") Scroll();
        if (command == "QUERY_RANKING") QueryRanking();
        if (command == "QUERY_SUBMISSION") QuerySubmission();
    }
    return 0;
}

void Start() {
    string DURATION, PROBLEM;
    int duration_time_temp, problem_count_temp;
    cin >> DURATION >> duration_time_temp >> PROBLEM >> problem_count_temp;
    if (status) {
        cout << "[Error]Start failed: competition has started.\n";
    }
    else {
        cout << "[Info]Competition starts.\n";
        status = 1;
        int ranking = 0;
        duration_time = duration_time_temp;
        problem_count = problem_count_temp;
        for (const Team& t : team) {
            teams[t.number].rank = ++ranking; // 初始排名
        }
    }
}

void Addteam() { // O(log N)
    string team_name;
    cin >> team_name;
    if (status) cout << "[Error]Add failed: competition has started.\n";
    else {
        if (shift[team_name]) cout << "[Error]Add failed: duplicated team name.\n";
        else {
            cout << "[Info]Add successfully.\n";
            shift[team_name] = ++team_cnt;
            teams[team_cnt] = {team_name, team_cnt};
            team.insert({team_name, team_cnt});
        }
    }
}

void Submit() { // O(log N)
    char problem_name;
    string team_name;
    string submit_status;
    string BY, WITH, AT;
    int time;
    cin >> problem_name >> BY >> team_name >> WITH >> submit_status >> AT >> time;
    
    submit_id++; // 提交编号
    int t = shift[team_name]; // 队伍编号
    int p = problem_name - 'A' + 1; // 题号
    int s = status_shift(submit_status); // 状态
    
    Submission sub{submit_id, p, s, time};
    Team &now_team = teams[t];
    now_team.submission[p][s] = sub; // 指定题目、指定状态
    now_team.submission[p][0] = sub; // 指定题目、任意状态
    now_team.submission[0][s] = sub; // 任意题目、指定状态
    now_team.submission[0][0] = sub; // 任意题目、任意状态
    
    Problem &now_problem = now_team.problem[p];
    if (now_problem.first_pass_time == -1) { // 之前没AC
        if (s == 1) { // AC
            if (!frozen) {
                rank_change = 1;
                rank_changed[t] = 1;
            }
            now_problem.first_pass_time = time;
            now_team.pass_num++;
            now_team.penalty += time + 20 * now_problem.wrong_times;
            now_team.accept_time.push_back(time);
        }
        else {
            now_problem.wrong_times++;
        }
    }

    if (!frozen) { // 还没封榜 同步visible
        now_team.visible.pass_num = now_team.pass_num;
        now_team.visible.penalty = now_team.penalty;
        now_team.visible.accept_time = now_team.accept_time;
        for (int i = 1; i <= problem_count; i++) {
            now_team.visible.problem[i] = now_team.problem[i];
        }
    }
    else {
        if (now_team.visible.problem[p].first_pass_time == -1) {
            now_team.frozen_submit[p]++;
        }
    }
}

void Flush() { // O(Nlog N) 根据当前公开成绩重新计算排名
    // cout << "[Info]Flush scoreboard.\n";
    // 进行神奇操作
    if (!rank_change) {
        return;
    }
    vector<int> idx(team_cnt);
    for (int t = 1; t <= team_cnt; t++) {
        idx[teams[t].rank - 1] = t; // 存储原来顺序
    }

    vector<int> changed;
    vector<int> unchanged;

    for (int t : idx) {
        if (rank_changed[t]) {
            changed.push_back(t);
        }
        else {
            unchanged.push_back(t);
        }
    }
    // 给变化的队伍排序后merge
    sort(changed.begin(), changed.end(), cmp);
    vector<int> merged(team_cnt);
    merge(unchanged.begin(), unchanged.end(), changed.begin(), changed.end(), merged.begin(), cmp);
    // vector<int> id(team_cnt);
    // for (int i = 0; i < team_cnt; i++) {
    //     id[i] = i + 1;
    // }
    // sort(id.begin(), id.end(), cmp);
    int ranking = 0;
    for (int t : merged) {
        teams[t].rank = ++ranking;
        rank_changed[t] = 0;
    }
    rank_change = 0;
}

void Freeze() {
    if (frozen) {
        cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
    }
    else {
        cout << "[Info]Freeze scoreboard.\n";
        // 进行神奇操作
        frozen = 1;
        for (int t = 1; t <= team_cnt; t++) {
            Team &now_team = teams[t];
            now_team.visible.pass_num = now_team.pass_num;
            now_team.visible.penalty = now_team.penalty;
            now_team.visible.accept_time = now_team.accept_time;
            for (int i = 1; i <= problem_count; i++) {
                now_team.visible.problem[i] = now_team.problem[i];
            }
            for (int p = 1; p <= problem_count; p++) {
                now_team.frozen_submit[p] = 0; // 冻结后题p的提交次数
            }
        }
    }
}

void Print() {
    int ranking = 0;
    for (const Team& t : team) {
        const Team& now_team = teams[t.number];
        cout << now_team.name << " " << ++ranking << " " << 
                now_team.visible.pass_num << " " << now_team.visible.penalty << " ";
        for (int p = 1; p <= problem_count; p++) {
            const Problem &now_problem = now_team.visible.problem[p];
            int errors = now_problem.wrong_times;
            if (now_problem.first_pass_time != -1) {
                cout << "+";
                if (errors) {
                    cout << errors;
                }
            }
            else {
                if (now_team.frozen_submit[p]) { // 冻结
                    if (errors) {
                        cout << '-';
                    }
                    cout << errors << '/' << now_team.frozen_submit[p];
                }
                else {
                    if (errors) {
                        cout << "-" << errors;
                    }
                    else {
                        cout << '.';
                    }
                }
            }
            cout << " ";
        }
        cout << "\n";
    }
}

void Update() {
    set<Team> temp; // 临时保存有冻结题目的队伍
    for (const auto &t : team) {
        bool flag = 0;
        for (int p = 1; p <= problem_count; p++) {
            if (t.frozen_submit[p]) {
                flag = 1;
                break;
            }
        }
        if (flag) temp.insert(t);
    }

    while (temp.size()) {
        auto it = prev(temp.end()); 
        Team old = *it; // 取出最后一只队伍
        temp.erase(it);
        team.erase(old);
        int t = old.number;
        int now_problem = 0;
        Team &now_team = teams[t];
        for (int p = 1; p <= problem_count; p++) {
            if (now_team.frozen_submit[p]) { // 找到解冻的题
                now_problem = p;
                break;
            }
        }
        // 这里可以优化吗
        if (now_team.problem[now_problem].first_pass_time != -1) { // 冻结时AC
            now_team.visible.pass_num++;
            now_team.visible.penalty += now_team.problem[now_problem].first_pass_time + 
                                        20 * now_team.problem[now_problem].wrong_times;
            now_team.visible.accept_time.push_back(now_team.problem[now_problem].first_pass_time);
            for (int i = now_team.visible.accept_time.size() - 1; i > 0; i--) {
                if (now_team.visible.accept_time[i] < now_team.visible.accept_time[i - 1]) { // 保持升序排序
                    swap(now_team.visible.accept_time[i], now_team.visible.accept_time[i - 1]);
                }
            }
        }
        now_team.visible.problem[now_problem] = now_team.problem[now_problem];
        now_team.frozen_submit[now_problem] = 0;

        auto new_it = team.insert(now_team).first; // insert返回值是pair
        auto below = next(new_it); // 现在的后继
        if (below != team.end() && *below < old) {
            cout << now_team.name << " " << (*below).name << " " << 
                    now_team.visible.pass_num << " " << now_team.visible.penalty << "\n";
        }

        bool flag = 0;
        for (int p = 1; p <= problem_count; p++) {
            if (now_team.frozen_submit[p]) {
                flag = 1;
                break;
            }
        }
        if (flag) {
            temp.insert(now_team);
            // team.insert(now_team);
        }
    }

    int ranking = 0;
    for (const auto& t : team) {
        teams[t.number].rank = ++ranking;
        rank_changed[t.number] = 0;
    }
    rank_change = 0;
    frozen = 0;
}

void Scroll() { // O(Nlog N)
    if (!frozen) {
        cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
    }
    else {
        cout << "[Info]Scroll scoreboard.\n";
        Flush(); 
        team.clear();
        for (int i = 1; i <= team_cnt; ++i) {
            team.insert(teams[i]);
        }
        Print();
        Update();
        Print();
    }
}

void QueryRanking() { // O(log N)
    string team_name;
    cin >> team_name;
    if (!shift[team_name]) {
        cout << "[Error]Query ranking failed: cannot find the team.\n";
    }
    else {
        cout << "[Info]Complete query ranking.\n";
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }
        // 进行神秘操作
        cout << team_name << " NOW AT RANKING " << teams[shift[team_name]].rank << "\n";
    }
}

void QuerySubmission() { // O(log N)
    string team_name, WHERE, PROBLEM, AND, STATUS;
    cin >> team_name >> WHERE >> PROBLEM >> AND >> STATUS;
    if (!shift[team_name]) {
        cout << "[Error]Query submission failed: cannot find the team.\n";
        return;
    }
    cout << "[Info]Complete query submission.\n";
    const Team& now_team = teams[shift[team_name]];
    /* 
        now_team.submission[p][s] = sub; // 指定题目、指定状态
        now_team.submission[p][0] = sub; // 指定题目、任意状态
        now_team.submission[0][s] = sub; // 任意题目、指定状态
        now_team.submission[0][0] = sub; // 任意题目、任意状态
    */
    int s = status_shift(STATUS.substr(7));
    int p = 0;
    if (PROBLEM != "PROBLEM=ALL") {
        p = PROBLEM[8] - 'A' + 1;
    }
    if (!now_team.submission[p][s].id) { // 没有提交记录
        cout << "Cannot find any submission.\n";
    }
    else {
        // [team_name] [problem_name] [status] [time]
        cout << team_name << " " << char('A' + now_team.submission[p][s].problem - 1) << " " << shift_status(now_team.submission[p][s].status) << " " << now_team.submission[p][s].time << "\n";
    }
}