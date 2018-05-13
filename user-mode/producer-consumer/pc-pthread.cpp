#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>
#include <pthread.h>

class ProducerConsumer {
	enum {
		NR_SLOTS = 4,
	};

public:
	ProducerConsumer();
	~ProducerConsumer();

	bool isEmpty() const {
		return (m_rd_pt == m_wr_pt);
	}

	bool isFull() const {
		return (m_wr_pt+1)%NR_SLOTS==m_rd_pt;
	}

	inline void lock() {
		::pthread_mutex_lock(&m_lock);
	}

	inline void unlock() {
		::pthread_mutex_unlock(&m_lock);
	}

	void readWait() {
		::pthread_cond_wait(&m_read_queue, &m_lock);
	}

	void writeWait() {
		::pthread_cond_wait(&m_write_queue, &m_lock);
	}

	void wakeRead() {
		::pthread_cond_signal(&m_read_queue);
	}

	void wakeWrite() {
		::pthread_cond_signal(&m_write_queue);
	}

	void produceData();
	void consumeData();

	static void *read(void *);
	static void *write(void *);

	static void run();

private:
	const int m_fd;
	int m_buf[NR_SLOTS];
	int m_rd_pt;
	int m_wr_pt;

	int m_data;

	::pthread_mutex_t m_lock;
	::pthread_cond_t  m_read_queue;
	::pthread_cond_t  m_write_queue;
};

ProducerConsumer::ProducerConsumer():m_fd(0), m_rd_pt(0), m_wr_pt(0), m_data(0) {
	::pthread_mutex_init(&m_lock, nullptr);
	::pthread_cond_init(&m_read_queue, nullptr);
	::pthread_cond_init(&m_write_queue, nullptr);
}

ProducerConsumer::~ProducerConsumer() {
	if (m_fd>0) {
		::close(m_fd);
	}
}

void *ProducerConsumer::write(void *data) {
	const pthread_t pid = ::pthread_self();
	::printf("debug ---> %ld\n", pid);

	ProducerConsumer *pc = static_cast<ProducerConsumer *>(data);

	while (true) {
		pc->lock();

		while (pc->isFull()) {
			/**
			 *  buffer is full,
			 *    - wait in the write queue
			 *    - release the lock
			 */
			pc->writeWait();

			/* got a signal and clocked */
		}

		pc->produceData();
		pc->wakeRead();

		pc->unlock();
	}

	return nullptr;
}

void *ProducerConsumer::read(void *data) {
	const pthread_t pid = ::pthread_self();
	ProducerConsumer *pc = static_cast<ProducerConsumer *>(data);

	::printf("debug ---> %ld\n", pid);

	while (true) {
		pc->lock();
		while (pc->isEmpty()) {
			pc->readWait();

			/* wait read queue, release lock */
		}

		/* consume the data */
		pc->consumeData();

		pc->wakeWrite();
		pc->unlock();
	}

	return nullptr;
}

void ProducerConsumer::produceData() {
	m_data += 1;
	m_buf[m_wr_pt] = m_data;
	m_wr_pt = (m_wr_pt + 1)%NR_SLOTS;
}

void ProducerConsumer::consumeData() {
	::sleep(1);
	::printf("debug ---> %d\n", m_buf[m_rd_pt]);
	m_rd_pt = (m_rd_pt + 1)%NR_SLOTS;
}

void ProducerConsumer::run() {
	//const char device[] = "/dev/test";
	ProducerConsumer pc;

	pthread_t pid[2];
	pthread_attr_t *pthread_attr = nullptr;

	pthread_create(&pid[0], pthread_attr, ProducerConsumer::write, &pc);
	pthread_create(&pid[1], pthread_attr, ProducerConsumer::read, &pc);

	pthread_join(pid[0], nullptr);
	pthread_join(pid[1], nullptr);
	pthread_exit(nullptr);

}

const char *reverse_string(const char *st) {
	const int len = ::strlen(st);
	char *buf = static_cast<char *>(::malloc(len-1));

	int j = len-1;
	for(int i=0; i<len; i++, j--) {
		buf[i] = st[j];
	}

	return buf;
}

void test_reverse_string( ) {
	::printf("%s\n", reverse_string("0123456789"));
}

int main(int argc, char *argv[]) {
    //ProducerConsumer::run();
	test_reverse_string();

    return 0;
}
