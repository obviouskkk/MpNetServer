#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

#include "config.h"
#include "List.h"

#define CONFIG_VAL_SIZE 4096

static list_t ini_key_head[256];
static int has_init = 0; 

typedef struct config_pair {
	struct list_node list;
	char* val;
	char  key[];
} config_pair_t;


static inline uint32_t r5hash(const char* p)
{
	uint32_t h = 0;
	while (*p) {
		h = h * 11 + (*p << 4) + (*p >> 4); // 123 max
		p++;
	}
	return h;
}

static inline struct list_node* config_key_hash(const char* key) 
{	
	return &ini_key_head[r5hash(key) & (256 - 1)];
}

static inline int hash_key(const char *key)
{
	return r5hash(key) & (256 - 1);
}

//将key，value加入到哈希表里,
int config_append_value(const char* key, const char* val) 
{
	int val_len = strlen(val) + 1;
	if (key == NULL || val == NULL || val_len > CONFIG_VAL_SIZE)
		return -1;
	
	int hash = hash_key(key);
	list_t* p = ini_key_head[hash].next;
    //找一下有没有重复的key值
	while (p != &ini_key_head[hash]) { //从当前位置的next找，直到当前位置的前一个
		config_pair_t* mc = list_entry(p, config_pair_t, list);
		if (strlen(mc->key) == strlen(key) && strcmp(mc->key, key) == 0) {
            //如果有两个key值一样，返回-1 去update去
			return -1;
		}
		p = p->next;
	}

	int len = strlen(key) + 1;
	struct config_pair* mc = malloc(sizeof(struct config_pair) + len);
	if (!mc) {
		return -1;
	}
	memcpy(mc->key, key, len);
	mc->val = (char*)malloc(CONFIG_VAL_SIZE);
	if (!mc->val) {
		free(mc);
		return -1;
	}
	memcpy(mc->val, val, val_len);
	list_push_front(&mc->list, &ini_key_head[hash]);

	return 0;
}

int config_update_value(const char* key, const char* val)
{
	int val_len = strlen(val) + 1;
	if (key == NULL || val == NULL || val_len > CONFIG_VAL_SIZE)
		return -1;

	int hash = hash_key(key);
	list_t* p = ini_key_head[hash].next;
	while (p != &ini_key_head[hash]) {
		config_pair_t* mc = list_entry(p, config_pair_t, list);
		if (strlen(mc->key) == strlen(key) && strcmp(mc->key, key) == 0 
			&& (strlen(mc->val) != strlen(val) || strcmp(mc->val, val) != 0)) {
			memcpy(mc->val, val, val_len);//key相等，val不相等，则update
            return 0;
		}
		p = p->next;
	}
	return -1;
}

static int  config_reset_or_add_value( const char *  k,  const char *  v){
	if(config_append_value(k, v) == -1) {
		if(config_update_value(k, v) == -1) {
			return -1;
		}
	}
	return 0;
}

static int parse_config(char* buffer)
{
	static const char myifs[256]= { [' '] = 1, ['\t'] = 1,\
		['\r'] = 1, ['\n'] = 1, ['='] = 1 };

	char*  field[2];
	char*  start = buffer;
	size_t len   = strlen(buffer);
	while (buffer + len > start) {
		char* end = strchr(start, '\n'); //首次出现\n的位置
		if (end) {
			*end = '\0';
		}
        //如果不是"#"，并且字符串切割成功；
		if ((*start != '#') && (str_split(myifs, start, field, 2) == 2)) {
			if (config_reset_or_add_value(field[0], field[1]) == -1) {
				return -1;
			}
		}
		if (end) {
			start = end + 1;
		} else {
			break;
		}
	}
	return 0;
}

//创建内存映射，成功返回映射长度并把文件内容读到buf里，失败返回-1
int mmap_config_file(const char* file_name, char** buf)
{
	int ret_code = -1;

	int fd = open(file_name, O_RDONLY);
	if (fd == -1) {
		return -1;
	}

	int len = lseek(fd, 0L, SEEK_END);//文件有多长
	lseek(fd, 0L, SEEK_SET);
	*buf = mmap(0, len + 1, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (*buf != MAP_FAILED) {
		read(fd, *buf, len);
		(*buf)[len] = 0;
		ret_code = len + 1;
	}
	close(fd);

	return ret_code;
}


int config_init(const char* file_name)
{

	int ret_code = -1;

	if (!has_init) {
		int i;
		for (i = 0; i < 256; i++) {
			INIT_LIST_HEAD(&ini_key_head[i]);
		}
		has_init = 1;
	}

		char* buf;
		int   len = mmap_config_file(file_name, &buf);
		if (len > 0) {
			ret_code = parse_config(buf);
			munmap(buf, len);
		}

	return ret_code;
}

//字符串切割 例如 ”run_style background \n“ 分取到别run_style background
//读取line这一行，取两个值，存在field中，返回实际取到的数目
int str_split(const char* ifs, char* line, char* field[], int n)
{
	static const char default_ifs[256]= { [' '] = 1, ['\t'] = 1, ['\r'] = 1, ['\n'] = 1, ['='] = 1 };

	if (ifs == 0) {
		ifs = default_ifs;
	}

	int i = 0;
	while (1) {
		while (ifs[(unsigned char)*line]) {
			++line;
		}
        //找到不是ifs的字符，如果直接找到\0退出循环
		if (*line == '\0') {
			break;
		}
		field[i++] = line;
		// 已经取到了足够的值,第二次
		if (i >= n) {
			line += strlen(line) - 1;
			while (ifs[(unsigned char)*line]) {//检查一下中间有没有ifs
				--line;
			}
			line[1] = '\0';//*(line+1) = '\0'
			break;
		}
		// 第一次 给尾部加一个 ‘\0’,
		while (*line && !ifs[(unsigned char)*line]) {
			++line;
		}
		if (!*line) {
			break;
		}
		*line++ = '\0';
	}
	return i;
}

int config_get_intval(const char* key, int def)
{

	char* val = config_get_strval(key);
	if (val == 0) {
		return def;
	}
	return atoi(val);
}

char* config_get_strval(const char* key)
{
	struct config_pair* mc;

	list_t* hlist = config_key_hash(key);
	list_for_each_entry(mc, hlist, list) {
		if (!strcmp(key, mc->key)) {
			return mc->val;
		}
	}

	return 0;
}


void config_exit()
{
	int i;
	if (!has_init)
		return;

	for (i = 0; i < 256 ; ++i) {
		list_t* p = ini_key_head[i].next;
		while (p != &ini_key_head[i]) {
			config_pair_t* mc = list_entry(p, config_pair_t, list);
			p = p->next;
			free(mc->val);
			free(mc);
		}
	}
	has_init = 0;
}







