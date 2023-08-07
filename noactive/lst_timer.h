#ifndef LST_TIMER
#define LST_TIMER

#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 64
class util_timer;   // ǰ������

// �û����ݽṹ
struct client_data
{
    sockaddr_in address;    // �ͻ���socket��ַ
    int sockfd;             // socket�ļ�������
    char buf[ BUFFER_SIZE ];    // ������
    util_timer* timer;          // ��ʱ��
};

// ��ʱ����
class util_timer {
public:
    util_timer() : prev(NULL), next(NULL){}

public:
   time_t expire;   // ����ʱʱ�䣬����ʹ�þ���ʱ��
   void (*cb_func)( client_data* ); // ����ص��������ص���������Ŀͻ����ݣ��ɶ�ʱ����ִ���ߴ��ݸ��ص�����
   client_data* user_data; 
   util_timer* prev;    // ָ��ǰһ����ʱ��
   util_timer* next;    // ָ���һ����ʱ��
};

// ��ʱ����������һ������˫�������Ҵ���ͷ�ڵ��β�ڵ㡣
class sort_timer_lst {
public:
    sort_timer_lst() : head( NULL ), tail( NULL ) {}
    // ��������ʱ��ɾ���������еĶ�ʱ��
    ~sort_timer_lst() {
        util_timer* tmp = head;
        while( tmp ) {
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    }
    
    // ��Ŀ�궨ʱ��timer��ӵ�������
    void add_timer( util_timer* timer ) {
        if( !timer ) {
            return;
        }
        if( !head ) {
            head = tail = timer;
            return; 
        }
        /* ���Ŀ�궨ʱ���ĳ�ʱʱ��С�ڵ�ǰ���������ж�ʱ���ĳ�ʱʱ�䣬��Ѹö�ʱ����������ͷ��,��Ϊ�����µ�ͷ�ڵ㣬
           �������Ҫ�������غ��� add_timer(),�������������к��ʵ�λ�ã��Ա�֤������������� */
        if( timer->expire < head->expire ) {
            timer->next = head;
            head->prev = timer;
            head = timer;
            return;
        }
        add_timer(timer, head);
    }
    
    /* ��ĳ����ʱ�������仯ʱ��������Ӧ�Ķ�ʱ���������е�λ�á��������ֻ���Ǳ������Ķ�ʱ����
    ��ʱʱ���ӳ�����������ö�ʱ����Ҫ�������β���ƶ���*/
    void adjust_timer(util_timer* timer)
    {
        if( !timer )  {
            return;
        }
        util_timer* tmp = timer->next;
        // �����������Ŀ�궨ʱ�����������β�������߸ö�ʱ���µĳ�ʱʱ��ֵ��ȻС������һ����ʱ���ĳ�ʱʱ�����õ���
        if( !tmp || ( timer->expire < tmp->expire ) ) {
            return;
        }
        // ���Ŀ�궨ʱ���������ͷ�ڵ㣬�򽫸ö�ʱ����������ȡ�������²�������
        if( timer == head ) {
            head = head->next;
            head->prev = NULL;
            timer->next = NULL;
            add_timer( timer, head );
        } else {
            // ���Ŀ�궨ʱ�����������ͷ�ڵ㣬�򽫸ö�ʱ����������ȡ����Ȼ�������ԭ������λ�ú�Ĳ���������
            timer->prev->next = timer->next;
            timer->next->prev = timer->prev;
            add_timer( timer, timer->next );
        }
    }
    // ��Ŀ�궨ʱ�� timer ��������ɾ��
    void del_timer( util_timer* timer )
    {
        if( !timer ) {
            return;
        }
        // �����������������ʾ������ֻ��һ����ʱ������Ŀ�궨ʱ��
        if( ( timer == head ) && ( timer == tail ) ) {
            delete timer;
            head = NULL;
            tail = NULL;
            return;
        }
        /* ���������������������ʱ������Ŀ�궨ʱ���������ͷ�ڵ㣬
         �������ͷ�ڵ�����Ϊԭͷ�ڵ����һ���ڵ㣬Ȼ��ɾ��Ŀ�궨ʱ���� */
        if( timer == head ) {
            head = head->next;
            head->prev = NULL;
            delete timer;
            return;
        }
        /* ���������������������ʱ������Ŀ�궨ʱ���������β�ڵ㣬
        �������β�ڵ�����Ϊԭβ�ڵ��ǰһ���ڵ㣬Ȼ��ɾ��Ŀ�궨ʱ����*/
        if( timer == tail ) {
            tail = tail->prev;
            tail->next = NULL;
            delete timer;
            return;
        }
        // ���Ŀ�궨ʱ��λ��������м䣬�����ǰ��Ķ�ʱ������������Ȼ��ɾ��Ŀ�궨ʱ��
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        delete timer;
    }

    /* SIGALARM �ź�ÿ�α������������źŴ�������ִ��һ�� tick() �������Դ��������ϵ�������*/
    void tick() {
        if( !head ) {
            return;
        }
        printf( "timer tick\n" );
        time_t cur = time( NULL );  // ��ȡ��ǰϵͳʱ��
        util_timer* tmp = head;
        // ��ͷ�ڵ㿪ʼ���δ���ÿ����ʱ����ֱ������һ����δ���ڵĶ�ʱ��
        while( tmp ) {
            /* ��Ϊÿ����ʱ����ʹ�þ���ʱ����Ϊ��ʱֵ�����Կ��԰Ѷ�ʱ���ĳ�ʱֵ��ϵͳ��ǰʱ�䣬
            �Ƚ����ж϶�ʱ���Ƿ���*/
            if( cur < tmp->expire ) {
                break;
            }

            // ���ö�ʱ���Ļص���������ִ�ж�ʱ����
            tmp->cb_func( tmp->user_data );
            // ִ���궨ʱ���еĶ�ʱ����֮�󣬾ͽ�����������ɾ��������������ͷ�ڵ�
            head = tmp->next;
            if( head ) {
                head->prev = NULL;
            }
            delete tmp;
            tmp = head;
        }
    }

private:
    /* һ�����صĸ����������������е� add_timer ������ adjust_timer ��������
    �ú�����ʾ��Ŀ�궨ʱ�� timer ��ӵ��ڵ� lst_head ֮��Ĳ��������� */
    void add_timer(util_timer* timer, util_timer* lst_head)  {
        util_timer* prev = lst_head;
        util_timer* tmp = prev->next;
        /* ���� list_head �ڵ�֮��Ĳ�������ֱ���ҵ�һ����ʱʱ�����Ŀ�궨ʱ���ĳ�ʱʱ��ڵ�
        ����Ŀ�궨ʱ������ýڵ�֮ǰ */
        while(tmp) {
            if( timer->expire < tmp->expire ) {
                prev->next = timer;
                timer->next = tmp;
                tmp->prev = timer;
                timer->prev = prev;
                break;
            }
            prev = tmp;
            tmp = tmp->next;
        }
        /* ��������� lst_head �ڵ�֮��Ĳ���������δ�ҵ���ʱʱ�����Ŀ�궨ʱ���ĳ�ʱʱ��Ľڵ㣬
           ��Ŀ�궨ʱ����������β��������������Ϊ�����µ�β�ڵ㡣*/
        if( !tmp ) {
            prev->next = timer;
            timer->prev = prev;
            timer->next = NULL;
            tail = timer;
        }
    }

private:
    util_timer* head;   // ͷ���
    util_timer* tail;   // β���
};

#endif
