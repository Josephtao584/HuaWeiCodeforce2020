#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdlib>
#include <algorithm>
#include <fcntl.h>
#include <pthread.h>
#include <queue>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <set>
#include <atomic>
#include <pthread.h>

typedef uint32_t ui;
typedef uint64_t ul;
#define A(N) t = (1ULL << (32 + N / 5 * N * 53 / 16)) / uint32_t(1e##N) + 1 - N / 9, t *= u, t >>= N / 5 * N * 53 / 16, t += N / 5 * 4

#if 0
#define D(N) b[N] = char(t >> 32) + '0'
#define E t = 10ULL * uint32_t(t)
#define L0 b[0] = char(u) + '0'
#define L1 A(1), D(0), E, D(1)
#define L2 A(2), D(0), E, D(1), E, D(2)
#define L3 A(3), D(0), E, D(1), E, D(2), E, D(3)
#define L4 A(4), D(0), E, D(1), E, D(2), E, D(3), E, D(4)
#define L5 A(5), D(0), E, D(1), E, D(2), E, D(3), E, D(4), E, D(5)
#define L6 A(6), D(0), E, D(1), E, D(2), E, D(3), E, D(4), E, D(5), E, D(6)
#define L7 A(7), D(0), E, D(1), E, D(2), E, D(3), E, D(4), E, D(5), E, D(6), E, D(7)
#define L8 A(8), D(0), E, D(1), E, D(2), E, D(3), E, D(4), E, D(5), E, D(6), E, D(7), E, D(8)
#define L9 A(9), D(0), E, D(1), E, D(2), E, D(3), E, D(4), E, D(5), E, D(6), E, D(7), E, D(8), E, D(9)
#else
static const uint16_t s_100s[] = {
        '00', '10', '20', '30', '40', '50', '60', '70', '80', '90',
        '01', '11', '21', '31', '41', '51', '61', '71', '81', '91',
        '02', '12', '22', '32', '42', '52', '62', '72', '82', '92',
        '03', '13', '23', '33', '43', '53', '63', '73', '83', '93',
        '04', '14', '24', '34', '44', '54', '64', '74', '84', '94',
        '05', '15', '25', '35', '45', '55', '65', '75', '85', '95',
        '06', '16', '26', '36', '46', '56', '66', '76', '86', '96',
        '07', '17', '27', '37', '47', '57', '67', '77', '87', '97',
        '08', '18', '28', '38', '48', '58', '68', '78', '88', '98',
        '09', '19', '29', '39', '49', '59', '69', '79', '89', '99',
};

#define W(N, I) *(uint16_t*)&b[N] = s_100s[I]
#define Q(N) b[N] = char((10ULL * uint32_t(t)) >> 32) + '0'
#define D(N) W(N, t >> 32)
#define E t = 100ULL * uint32_t(t)
#define L0 b[0] = char(u) + '0'
#define L1 W(0, u)
#define L2 A(1), D(0), Q(2)
#define L3 A(2), D(0), E, D(2)
#define L4 A(3), D(0), E, D(2), Q(4)
#define L5 A(4), D(0), E, D(2), E, D(4)
#define L6 A(5), D(0), E, D(2), E, D(4), Q(6)
#define L7 A(6), D(0), E, D(2), E, D(4), E, D(6)
#define L8 A(7), D(0), E, D(2), E, D(4), E, D(6), Q(8)
#define L9 A(8), D(0), E, D(2), E, D(4), E, D(6), E, D(8)
#endif

//#define _TEST

#define LQ(N) (L##N, b += N + 1)
#define LP(F) (u<100 ? u<10 ? F(0) : F(1) : u<1000000 ? u<10000 ? u<1000 ? F(2) : F(3) : u<100000 ? F(4) : F(5) : u<100000000 ? u<10000000 ? F(6) : F(7) : u<1000000000 ? F(8) : F(9))
#define MAX_EDGE 2000001
#define MAX_NODE 4000000
#define THREAD_COUNT 4
#define CYCLE_3_NUM 5000000
#define CYCLE_4_NUM 5000000
#define CYCLE_5_NUM 5000000
#define CYCLE_6_NUM 8000000
#define CYCLE_7_NUM 10000000
#define ANS_BUFFER_LEN 2000000000
#define JOBS_PER_EPOCH 5 // 默认参数 5
#define LEFT 3
#define RIGHT 5

using namespace std;
ui node_cnt = 0;           //节点数
char *readBuf;//存放读入数据，类似mmap


ui index_id[MAX_NODE];    //索引对应id
struct gstruct {           //存儿子和对应权值
    ui son = 0;
    ui val = 0;  // changelog：修改val的类型为ui, 保证结构体gstruct的大小为8字节，64bit，对齐机器字。
}__attribute__((packed));

struct edge_struct {
    ui u;
    ui v;
    ui val;
};
edge_struct *readData;

struct thread_write {
    ui start; // 起始位置
    ui len; // 实际写入的字节数目
    ui *thread_cycle_cnt;
    char *thread_buffer;
}__attribute__((aligned));


atomic_uint32_t atomic_count(0);

vector<vector<gstruct>> g;        //存放儿子节点的指针
vector<vector<gstruct>> f;        //存放父节点的指针
ui *cycle3[THREAD_COUNT], *cycle4[THREAD_COUNT], *cycle5[THREAD_COUNT], *cycle6[THREAD_COUNT], *cycle7[THREAD_COUNT];   //存放对应环
ui *all_cycle[THREAD_COUNT * 5] = {NULL};//存放所有结果
ui cycle_len[20] = {0};     //每个结果的环数
ui num_len[20] = {0};       //每个结果的数字个数
ui *merge_cycle;            //归并后的数组
ui sum_len = 0;             //总字节数
ui cycle_cnt = 0;           //总环数

char *u32toa(uint32_t u, char *b) {
    uint64_t t;
    return LP(LQ);
}

bool cmp(edge_struct a, edge_struct b) {
    if (a.u != b.u) {
        return a.u < b.u;
    } else {
        return a.v < b.v;
    }

}

void loadfile(string &filePath) {
    ifstream fileRead(filePath, ios::binary | ios::in);
#ifdef _TEST
    if (!fileRead.is_open()) {
        cout << "file read error!" << endl;
        return;
    }
#endif
    readBuf = new char[MAX_EDGE * 3 * 10]; //存放数据
    readData = new edge_struct[MAX_EDGE];
    int char_len = fileRead.seekg(0, ios::end).tellg(); //本次读入的总字节数
    fileRead.seekg(0, ios::beg).read(readBuf, static_cast<streamsize>(char_len));
    fileRead.close();

    ui edge_cnt = 0;
    char *p = readBuf;
    char *endP = readBuf + char_len;
    ui u, v, val;
    unordered_map<ui, ui> id_index;
    while (p != endP) {
        u = 0;
        while (*p != ',') {
            u = (u << 1) + (u << 3) + *(p++) - 48;
        }
        p++; // 跳过 ','
        v = 0;
        while (*p != ',') {
            v = (v << 1) + (v << 3) + *(p++) - 48;
        }
        p++; // 跳过','
        val = 0;
#ifndef _TEST
        //华为给的数据集每行结束为\r\n，这么读
        while (*p != '\r') {
            val = (val << 1) + (val << 3) + *(p++) - 48;
        }
        p+=2; // 跳过 \r\n
#endif
#ifdef _TEST
        //其他数据集每行结束为\n,这么读
        while (*p != '\n') {
            val = (val << 1) + (val << 3) + *(p++) - 48;
        }
        p++; // 跳过\n
#endif
        // 本轮读取的数组放到buffer中
        readData[edge_cnt].u = u;
        readData[edge_cnt].v = v;
        readData[edge_cnt].val = val;
        // 读取了三个数，记录数+1
        edge_cnt++;
    }
    sort(readData, readData + edge_cnt, cmp);
    readData[edge_cnt].u = 0;
    ui data;
    for (int i = 0; i < edge_cnt; ++i) {        //建索引
        data = readData[i].u;
        id_index[data] = node_cnt;
        index_id[node_cnt] = data;
        node_cnt++;
        while (readData[i].u == readData[i + 1].u)
            i++;
    }

    ui index_u, index_v;
    gstruct add;
    g.reserve(node_cnt);
    f.reserve(node_cnt);
    for (int j = 0; j < node_cnt; ++j) {
        g[j].reserve(50);
        f[j].reserve(50);
    }
    for (int i = 0; i < edge_cnt; ++i) {
        u = readData[i].u, v = readData[i].v, val = readData[i].val;
        if (id_index.find(v) == id_index.end())
            continue;
        index_u = id_index[u];
        index_v = id_index[v];
        add = {index_v, val};
        g[index_u].emplace_back(add);
        add = {index_u, val};
        f[index_v].emplace_back(add);
    }

    // 拓扑排序 ， 需要出度、入度， 开个数组计算一下， 在建索引那里
}

void add_cycle(ui point_stack[], int len, ui id) {
    ui add_len;
    ui add_id;
    switch (len) {
        case 3: {
            add_id = id;
            add_len = (cycle_len[add_id]++) * 3;
            for (int j = 0; j < len; ++j) {
                cycle3[id][add_len++] = point_stack[j];
            }
            break;
        }
        case 4: {
            add_id = id + 4;
            add_len = (cycle_len[add_id]++) * 4;
            for (int j = 0; j < len; ++j) {
                cycle4[id][add_len++] = point_stack[j];
            }
            break;
        }
        case 5: {
            add_id = id + 8;
            add_len = (cycle_len[add_id]++) * 5;
            for (int j = 0; j < len; ++j) {
                cycle5[id][add_len++] = point_stack[j];
            }
            break;
        }
        case 6: {
            add_id = id + 12;
            add_len = (cycle_len[add_id]++) * 6;
            for (int j = 0; j < len; ++j) {
                cycle6[id][add_len++] = point_stack[j];
            }
            break;
        }
        case 7: {
            add_id = id + 16;
            add_len = (cycle_len[add_id]++) * 7;
            for (int j = 0; j < len; ++j) {
                cycle7[id][add_len++] = point_stack[j];
            }
            break;
        }
    }
}

void startUp(ui &start, vector<bool> &set1, vector<bool> &set2, vector<bool> &set3, vector<ui> &set1_val, vector<bool> &visited) {
    visited[start] = true;
    ul val1, val2, val3;
    for (gstruct &g1 : f[start]) {
        ui data1 = g1.son;
        if (data1 < start)
            continue;
        val1 = g1.val;
        visited[data1] = true;
        set1[data1] = true;
        set2[data1] = true;
        set3[data1] = true;
        set1_val[data1] = val1;
        for (gstruct &g2 :f[data1]) {
            ui data2 = g2.son;
            if (data2 < start || visited[data2])
                continue;
            val2 = g2.val;
            if (val1 > LEFT * val2 || RIGHT * val1 < val2)
                continue;
            set2[data2] = true;
            set3[data2] = true;
            // visited[data2] = true;
            for (gstruct &g3 : f[data2]) {
                ui data3 = g3.son;
                if (data3 < start || visited[data3])
                    continue;
                val3 = g3.val;
                if (val2 > LEFT * val3 || RIGHT * val2 < val3)
                    continue;
                set3[data3] = true;
            }
            // visited[data2] = false;
        }
        visited[data1] = false;
    }
}

void startDown(ui &start, ui id, vector<bool> &set1, vector<bool> &set2, vector<bool> &set3, vector<ui> &set1_val,
               vector<bool> &visited, ui point_stack[]) {
    visited[start] = true;
    point_stack[0] = start;
    // float res = 0;
    ui son1, son2, son3, son4, son5, son6;
    ul val1, val2, val3, val4, val5, val6, addVal;
    for (gstruct &g1 :g[start]) {   //第一层
        son1 = g1.son;
        if (son1 < start)
            continue;
        val1 = g1.val;
        point_stack[1] = son1;
        visited[son1] = true;
        for (gstruct &g2 : g[son1]) { //第二层
            son2 = g2.son;
            if (son2 < start || visited[son2])
                continue;
            val2 = g2.val;
            // res = val2 / val1;
            if (val2 > LEFT * val1 || RIGHT * val2 < val1)
                continue;
            point_stack[2] = son2;
            if (set1[son2]) {      //找到长度3的环
                addVal = set1_val[son2];
                if (!((addVal > LEFT * val2) || (val1 > LEFT * addVal) || (RIGHT * addVal < val2) || (RIGHT * val1 < addVal)))
                    add_cycle(point_stack, 3, id);
            }
            visited[son2] = true;
            for (gstruct &g3 : g[son2]) {//第三层
                son3 = g3.son;
                if (son3 < start || visited[son3])
                    continue;
                val3 = g3.val;
                // res = val3 / val2;
                if (val3 > LEFT * val2 || RIGHT * val3 < val2)
                    continue;
                point_stack[3] = son3;
                if (set1[son3]) {
                    //找到4环
                    addVal = set1_val[son3];
                    if (!((addVal > LEFT * val3) || (val1 > LEFT * addVal) || (RIGHT * addVal < val3) || (RIGHT * val1 < addVal)))
                        add_cycle(point_stack, 4, id);
                }
                visited[son3] = true;
                for (gstruct &g4 :g[son3]) { //第四层
                    son4 = g4.son;
                    if (!set3[son4]) {
                        continue;           //判断能否继续
                    }
                    if (visited[son4])
                        continue;
                    val4 = g4.val;
//                    res = val4 / val3;
                    if (val4 > LEFT * val3 || RIGHT * val4 < val3)
                        continue;
                    point_stack[4] = son4;
                    if (set1[son4]) {
                        //找到5环
                        addVal = set1_val[son4];
                        if (!((addVal > LEFT * val4) || (val1 > LEFT * addVal) || (RIGHT * addVal < val4) || (RIGHT * val1 < addVal)))
                            add_cycle(point_stack, 5, id);
                    }
                    visited[son4] = true;
                    for (gstruct &g5 : g[son4]) {        //第五层
                        son5 = g5.son;
                        if (!set2[son5]) {     //判断第五层能否继续
                            continue;
                        }
                        if (visited[son5])
                            continue;

                        val5 = g5.val;
//                        res = val5 / val4;
                        if (val5 > 3 * val4 || RIGHT * val5 < val4)
                            continue;
                        point_stack[5] = son5;
                        if (set1[son5]) {
                            //找到6环
                            addVal = set1_val[son5];
                            if (!((val1 > LEFT * addVal) || (addVal > LEFT * val5) || (RIGHT * addVal < val5) ||
                                  (RIGHT * val1 < addVal)))
                                add_cycle(point_stack, 6, id);
                        }
                        //visited[son5] = true;
                        for (gstruct &g6 : g[son5]) {//第六层
                            son6 = g6.son;
                            if (set1[son6]) {
                                if (visited[son6])
                                    continue;
                                val6 = g6.val;
                                if (val6 > LEFT * val5 || RIGHT * val6 < val5)
                                    continue;
                                point_stack[6] = son6;
                                addVal = set1_val[son6];
                                if (!((val1 > LEFT * addVal) || (addVal > LEFT * val6) || (RIGHT * addVal < val6) ||
                                      (RIGHT * val1 < addVal)))
                                    add_cycle(point_stack, 7, id);           //找到长度7的环
                            }
                        }
                        //visited[son5] = false;
                    }
                    visited[son4] = false;
                }
                visited[son3] = false;
            }
            visited[son2] = false;
        }
        visited[son1] = false;
    }
    visited[start] = false;
}

void *single_dfs(void *arg) {
    vector<bool> visited(node_cnt, false), set1(node_cnt, false), set2(node_cnt, false), set3(node_cnt, false);
    vector<ui> set1_val(node_cnt);
    ui point_stack[7] = {0, 0, 0, 0, 0, 0, 0};
    ui id = *(ui *) arg;
    ui start;
    while ((start = atomic_count.fetch_add(JOBS_PER_EPOCH)) < node_cnt) {
        ui end = min(start + JOBS_PER_EPOCH, node_cnt);
        for (ui i = start; i < end; ++i) {
            fill(set1.begin(), set1.end(),false);
            fill(set2.begin(), set2.end(), false);
            fill(set3.begin(), set3.end(), false);
            startUp(i, set1, set2, set3, set1_val, visited);
            startDown(i, id, set1, set2, set3, set1_val, visited, point_stack);
        }
    }
    return nullptr;
}

void mul_thread_start() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    //调用single_dfs开始遍历
    pthread_t pthreads[THREAD_COUNT];
    ui ids[THREAD_COUNT];
    for (ui i = 0; i < THREAD_COUNT; ++i) {
        ids[i] = i;
        pthread_create(&pthreads[i], &attr, single_dfs, (void *) &ids[i]);
    }
    for (int i = 0; i < THREAD_COUNT; ++i) {
        pthread_join(pthreads[i], NULL);
    }
}

void get_sum_len() {
    for (int i = 0; i < 20; i++) {
        if (i < 4) {
            sum_len += cycle_len[i] * 3;
            cycle_cnt += cycle_len[i];
            num_len[i] = cycle_len[i] * 3;
        } else if (i >= 4 && i < 8) {
            sum_len += cycle_len[i] * 4;
            cycle_cnt += cycle_len[i];
            num_len[i] = cycle_len[i] * 4;
        } else if (i >= 8 && i < 12) {
            sum_len += cycle_len[i] * 5;
            cycle_cnt += cycle_len[i];
            num_len[i] = cycle_len[i] * 5;
        } else if (i >= 12 && i < 16) {
            sum_len += cycle_len[i] * 6;
            cycle_cnt += cycle_len[i];
            num_len[i] = cycle_len[i] * 6;
        } else {
            sum_len += cycle_len[i] * 7;
            cycle_cnt += cycle_len[i];
            num_len[i] = cycle_len[i] * 7;
        }
    }
}

ui getIndex(ui a, ui b, ui c, ui d) {
    ui res = 0;
    a < b ?: (a = b, res = 1);
    a < c ?: (a = c, res = 2);
    a < d ?: (res = 3);
    return res;
}

void mergeResult() {  //归并排序合并结果,放到一起
    ui count = 0;//merge_cycle指针
    for (int i = 0; i < 5; i++) {
        int col0 = 4 * i, col1 = 4 * i + 1, col2 = 4 * i + 2, col3 = 4 * i + 3; //列号
        int t[4] = {0};//指针指向四个数组
        ui tmp[4] = {0};//存放指针所指的值
        int minIndex;//最小值对应指针的索引
        int node_index, k;
        all_cycle[col0][num_len[col0]] = node_cnt + JOBS_PER_EPOCH;
        all_cycle[col1][num_len[col1]] = node_cnt + JOBS_PER_EPOCH;
        all_cycle[col2][num_len[col2]] = node_cnt + JOBS_PER_EPOCH;
        all_cycle[col3][num_len[col3]] = node_cnt + JOBS_PER_EPOCH;

        while (t[0] < num_len[col0] || t[1] < num_len[col1] || t[2] < num_len[col2] || t[3] < num_len[col3]) {
            tmp[0] = all_cycle[col0][t[0]];
            tmp[1] = all_cycle[col1][t[1]];
            tmp[2] = all_cycle[col2][t[2]];
            tmp[3] = all_cycle[col3][t[3]];
            minIndex = getIndex(tmp[0], tmp[1], tmp[2], tmp[3]);//min(min(tmp0,tmp1),min(tmp2,tmp3));
            node_index = tmp[minIndex];
            switch (i) {
                case 0: {
                    do {
                        for (k = 0; k < 3; k++) {//归并3的环
                            merge_cycle[count++] = all_cycle[4 * i + minIndex][t[minIndex]];
                            t[minIndex]++;
                        }
                    } while (all_cycle[THREAD_COUNT * i + minIndex][t[minIndex]] < node_index + JOBS_PER_EPOCH);
                    break;
                }
                case 1:
                    do {
                        for (k = 0; k < 4; k++) {//归并3的环
                            merge_cycle[count++] = all_cycle[4 * i + minIndex][t[minIndex]];
                            t[minIndex]++;
                        }
                    } while (all_cycle[THREAD_COUNT * i + minIndex][t[minIndex]] < node_index + JOBS_PER_EPOCH);
                    break;
                case 2:
                    do {
                        for (k = 0; k < 5; k++) {//归并3的环
                            merge_cycle[count++] = all_cycle[4 * i + minIndex][t[minIndex]];
                            t[minIndex]++;
                        }
                    } while (all_cycle[THREAD_COUNT * i + minIndex][t[minIndex]] < node_index + JOBS_PER_EPOCH);
                    break;
                case 3:
                    do {
                        for (k = 0; k < 6; k++) {//归并3的环
                            merge_cycle[count++] = all_cycle[4 * i + minIndex][t[minIndex]];
                            t[minIndex]++;
                        }
                    } while (all_cycle[THREAD_COUNT * i + minIndex][t[minIndex]] < node_index + JOBS_PER_EPOCH);
                    break;
                case 4:
                    do {
                        for (k = 0; k < 7; k++) {//归并3的环
                            merge_cycle[count++] = all_cycle[4 * i + minIndex][t[minIndex]];
                            t[minIndex]++;
                        }
                    } while (all_cycle[THREAD_COUNT * i + minIndex][t[minIndex]] < node_index + JOBS_PER_EPOCH);
                    break;
            }
        }
    }
}


void *writeToBuffer(void *arg) {
    auto *ti = (thread_write *) arg;
    ui start = ti->start;
    ui *cnt = ti->thread_cycle_cnt;
    char *buffer = ti->thread_buffer;
    char *c;
    uint8_t single_cnt;
    int j;
    ui len = 0; // 实际写入buffer的字节长度
    ui get_id;
    // 考虑每种环
    for (int i = 0; i < 5; ++i) {
        // 这种环的长度
        uint8_t single_cnt = i + 3;
        while (cnt[i] > 0) {
            // 如果当前种类有的话就写入buffer
            c = &buffer[len];
            get_id = index_id[merge_cycle[start++]];
            len += u32toa(get_id, c) - c;
            for (int j = 1; j < single_cnt; ++j) {
                buffer[len++] = ',';
                c = &buffer[len];
                get_id = index_id[merge_cycle[start++]];
                len += u32toa(get_id, c) - c;
            }
            buffer[len++] = '\n';
            --cnt[i];
        }
    }
    ti->len = len;
    return nullptr;
}


void writefile(string &writePath) {
    // 每种环的个数
    ui lens[5] = {0};
    for (int i = 0; i < THREAD_COUNT; ++i) {
        lens[0] += cycle_len[i];
        lens[1] += cycle_len[THREAD_COUNT + i];
        lens[2] += cycle_len[THREAD_COUNT * 2 + i];
        lens[3] += cycle_len[THREAD_COUNT * 3 + i];
        lens[4] += cycle_len[THREAD_COUNT * 4 + i];
    }
    ui len_per_thread = sum_len / THREAD_COUNT; // 每个线程默认分配的处理数目 (需要微调，保证不会切割环)
    ui buffer_size_per_thread = ANS_BUFFER_LEN / THREAD_COUNT; // 每个线程的buffer分配的默认大小

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_t write_threads[THREAD_COUNT]; // 定义工作线程
    thread_write infos[THREAD_COUNT]; // 定义工作线程的参数

    // 构造参数，并开启线程执行
    ui k = 0;
    for (int i = 0; i < THREAD_COUNT; ++i) {
        // 分配buffer以及它的长度
        infos[i].thread_buffer = new char[buffer_size_per_thread];
        infos[i].len = 0;
        // 记录当前线程每个长度的环要处理多少个
        infos[i].thread_cycle_cnt = new ui[5];
        // start就是上一个线程的结尾
        infos[i].start = k;
        // 默认分配
        if (i != THREAD_COUNT - 1) {
            ui mm = len_per_thread;
            // 调整k使得k分配不切割环
            for (int j = 0; j < 5; ++j) {
                // 单环数字数目为 3+j
                uint8_t single_len = 3 + j;
                // 如果当前种类的环还有，并且剩余的可分配额度大于单个长度
                if (lens[j] > 0 && mm >= single_len) {
                    ui assignment = min(mm / single_len, lens[j]);
                    lens[j] -= assignment;
                    infos[i].thread_cycle_cnt[j] = assignment;
                    mm -= assignment * single_len;
                } else {
                    infos[i].thread_cycle_cnt[j] = 0;
                }
            }
            // 减去没分配掉的
            k = k + len_per_thread - mm;
        } else {
            infos[i].thread_cycle_cnt = lens;
        }
        pthread_create(&write_threads[i], &attr, writeToBuffer, (void *) (&infos[i]));
    }

    // 等待所有线程都完成再退出
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(write_threads[i], NULL);
    }

    FILE *fp = fopen(writePath.c_str(), "w");
    // 写第一行
    fprintf(fp, "%u\n", cycle_cnt);
    //统一写入文件
    for (int i = 0; i < THREAD_COUNT; ++i) {
        fwrite(infos[i].thread_buffer, 1, infos[i].len, fp);
    }
}


int main() {
#ifdef _TEST
    auto start = chrono::steady_clock::now();
    string filePath = "/data/test_data1900.txt";
    string outPath = "/projects/student/result.txt";
#endif
#ifndef _TEST
    auto start = chrono::steady_clock::now();
    string filePath = "/data/test_data.txt";
    string outPath = "/projects/student/result.txt";
#endif
    for (int i = 0; i < THREAD_COUNT; ++i) {                //初始化结果数组
        cycle3[i] = new ui[CYCLE_3_NUM * 3];
        cycle4[i] = new ui[CYCLE_4_NUM * 4];
        cycle5[i] = new ui[CYCLE_5_NUM * 5];
        cycle6[i] = new ui[CYCLE_6_NUM * 6];
        cycle7[i] = new ui[CYCLE_7_NUM * 7];
        all_cycle[i] = cycle3[i];
        all_cycle[i + THREAD_COUNT] = cycle4[i];
        all_cycle[i + THREAD_COUNT * 2] = cycle5[i];
        all_cycle[i + THREAD_COUNT * 3] = cycle6[i];
        all_cycle[i + THREAD_COUNT * 4] = cycle7[i];
    }
#ifdef _TEST
    auto endalloc = chrono::steady_clock::now();
    cout << "alloc ram time " << chrono::duration<float>(endalloc - start).count() << endl;
#endif
    loadfile(filePath);
#ifdef _TEST
    auto endload = chrono::steady_clock::now();
    cout << "read time " << chrono::duration<float>(endload - endalloc).count() << endl;
#endif
    mul_thread_start();//遍历
#ifdef _TEST
    auto enddfs = chrono::steady_clock::now();
    cout << "dfs time " << chrono::duration<float>(enddfs - endload).count() << endl;
#endif
    get_sum_len();                                  //计算结果字节数量
    merge_cycle = new ui[sum_len];
    mergeResult();
#ifdef _TEST
    auto endmerge = chrono::steady_clock::now();
    cout << "merge time " << chrono::duration<float>(endmerge - enddfs).count() << endl;
#endif
    writefile(outPath);
#ifdef _TEST
    auto end = chrono::steady_clock::now();
    cout << "write time " << chrono::duration<float>(end - endmerge).count() << endl;
    cout << "all time " << chrono::duration<float>(end - start).count() << endl;
    cout << "finished!" << endl;
#endif
    exit(0);
}
