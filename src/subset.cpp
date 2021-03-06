#include "mf.h"

//这个文件的结构与convert.cpp基本一致，有些地方的注释请参考convert.cpp文件
//这里的subset主要是根据命令格式获得评分数据的一个子集
//好像这个subset函数没有被调用过

struct SubsetOption {
    char *src_path, *dst_path;

    //p_begin代表subset所需要的user id的最小值
    //p_end代表subset所需要的user id的最大值
    //q_begin代表subset所需要的item id的最小值
    //q_end代表subset所需要的item id的最大值
    int p_begin, p_end, q_begin, q_end, nr_rs;
	SubsetOption(int argc, char **argv);	
    static void exit_subset();
    ~SubsetOption();
};

/**
 * 构造函数
 */
SubsetOption::SubsetOption(int argc, char **argv) : p_begin(0), p_end(INT_MAX), q_begin(0), q_end(INT_MAX), nr_rs(0) {

    if(!strcmp(argv[1],"help")) exit_subset();

    int i;
	for(i=2; i<argc; i++) {
		if(argv[i][0]!='-') break; //如果第一个不是'-'，则退出循环（原因见于exit_subset()函数的usage）
		if(i+1>=argc) exit_subset(); //这是退出程序的条件

        if(!strcmp(argv[i], "-n")) nr_rs = atoi(argv[++i]); //根据命令参数给nr_rs赋值
        else if(!strcmp(argv[i], "-p")) { //根据命令参数给p_begin和p_end赋值
            char *p = strtok(argv[++i],",");
            p_begin = atoi(p);
            p = strtok(NULL,",");
            p_end = atoi(p);
        }
        else if(!strcmp(argv[i], "-q")) { //根据命令参数给q_begin和q_end赋值
            char *p = strtok(argv[++i],",");
            q_begin = atoi(p);
            p = strtok(NULL,",");
            q_end = atoi(p);
        }
        else { 
            fprintf(stderr,"Invalid option: %s\n", argv[i]); 
            exit_subset(); 
        }
	}

	if(i>=argc) exit_subset();

	src_path = argv[i++]; //这里i++为下面的if操作做准备
	
	if(i<argc) { //如果指定了output
		dst_path = new char[strlen(argv[i])+1];
		sprintf(dst_path,"%s",argv[i]);
	}
	else { //如果没有指定output，则根据src_path构造output
		char *p = strrchr(argv[i-1],'/'); //前面加了1，所以这里需要减1
		if(p==NULL)
			p = argv[i-1];
		else
			++p;
        dst_path = new char[65535];
		sprintf(dst_path,"%s",p);

        if(p_begin!=0 || p_end!=INT_MAX) sprintf(dst_path,"%s.p%d-%d",dst_path,p_begin,p_end);

        if(q_begin!=0 || q_end!=INT_MAX) sprintf(dst_path,"%s.q%d-%d",dst_path,q_begin,q_end);

        if(nr_rs!=0) sprintf(dst_path,"%s.n%d",dst_path,nr_rs);
	}
}

/** 
 * 展现命令参数格式，并退出程序
 */
void SubsetOption::exit_subset() {
    printf(
        "usage: mf subset [options] binary_file [output]\n"
        "\n"
        "options:\n" 
        "-n <number>: set the number of instances\n" 
        "-p <begin,end>: set the range of p (use 0 to indicate begin and end)\n" 
        "-q <begin,end>: set the range of q (use 0 to indicate begin and end)\n" 
    ); 
    exit(1);
}

/**
 * 析构函数
 */
SubsetOption::~SubsetOption() { 
    delete [] dst_path; 
}


Matrix* subset(int p_begin, int p_end, int q_begin, int q_end, int nr_rs, char *path) {

    Matrix *R = new Matrix(path), *R1 = new Matrix(); 

    std::vector<Node> nodes;

    if(p_end==INT_MAX) p_end = R->nr_us; 
    if(q_end==INT_MAX) q_end = R->nr_is;

    printf("Subseting %s... ", path); 
    fflush(stdout);

    Clock clock; 
    clock.tic();

    for(int rx=0; rx<R->nr_rs; rx++) {
        Node *r = &R->M[rx];
        if(r->uid>=p_begin && r->uid<p_end && r->iid>=q_begin && r->iid<q_end) 
            nodes.push_back(*r);

        if(nr_rs!=0 && (int)nodes.size()==nr_rs) break; //如果个数大于nr_rs，则不进行添加了
    } 

    R1->M = new Node[nodes.size()];

    for(auto it=nodes.begin(); it!=nodes.end(); it++) { //这里编号进行了调整，相减了
        it->uid -= p_begin; 
        it->iid -= q_begin;
        R1->M[it-nodes.begin()] = *it;
    }

    delete R->M; 
    R->M = R1->M; 
    R1->M = NULL; 
    delete R1;

    R->nr_us = p_end - p_begin; 
    R->nr_is = q_end - q_begin; 
    R->nr_rs = nodes.size();

    printf("done. %.2lf\n", clock.toc()); 
    fflush(stdout);

    return R;
}

/**
 * 析构函数
 * @param argc 命令个数
 * @param argv 命令个数
 */
void subset(int argc, char **argv) {

    SubsetOption *option = new SubsetOption(argc,argv);

    Matrix *R = subset(option->p_begin,option->p_end,option->q_begin,option->q_end,option->nr_rs,option->src_path);

    R->write(option->dst_path);

    delete option; 
    delete R;
}
