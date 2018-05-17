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

    enum {
        FD_READ = 0,
        FD_WRITE,
        NUM_OF_FDS,
    };

public:
	ProducerConsumer(const char *device, const int flag);
	~ProducerConsumer();

    //void close();
	void produceData();
	void consumeData();

	static void *read(void *);
	static void *write(void *);

	static void run();

private:
    const int m_fd;
	int m_data;
};


ProducerConsumer::ProducerConsumer(const char *device, const int flags)
   :m_fd(::open(device, flags)), m_data(0)
{
    if (m_fd<0) {
        throw std::runtime_error("open device failed\n");
    }
}

ProducerConsumer::~ProducerConsumer() {
	if (m_fd>0) {
		::close(m_fd);
	}
}

void *ProducerConsumer::write(void *data) {
	const pthread_t pid = ::pthread_self();
	::printf("debug ---> %ld\n", pid);

	ProducerConsumer pc("/dev/scu", ProducerConsumer::FD_WRITE);

	while (true) {
		pc.produceData();
	}

	return nullptr;
}

void *ProducerConsumer::read(void *data) {
	const pthread_t pid = ::pthread_self();
	::printf("debug ---> %ld\n", pid);

	ProducerConsumer pc("/dev/scu", ProducerConsumer::FD_READ);

	while (true) {
		/* consume the data */
		pc.consumeData();
	}

	return nullptr;
}

void ProducerConsumer::produceData() {
	m_data += 1;
	int n = ::write(m_fd, &m_data, sizeof(m_data));
	::printf("debug ---> write %d bytes\n", n);
}

void ProducerConsumer::consumeData() {
	int buf;
	int n = ::read(m_fd, &buf, sizeof(int));
	::printf("debug ---> number of read %d bytes:%d\n", n, buf);
}

void ProducerConsumer::run() {
	pthread_t pid[2];
	pthread_attr_t *pthread_attr = nullptr;

	pthread_create(&pid[0], pthread_attr, ProducerConsumer::write, NULL);
	pthread_create(&pid[1], pthread_attr, ProducerConsumer::read, NULL);

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

void reverse_string_recu(char *s, const int start, const int end) {

	if ( start==end || start > end) {
		return;
	}

	char ch = s[start];
	s[start] = s[end];
	s[end] = ch;
	reverse_string_recu(s, start+1, end-1);
}

void test_reverse_string( ) {
	//::printf("%s\n", reverse_string("0123456789"));
	char st[] = "0123456789";

	reverse_string_recu(st, 0, ::strlen(st)-1);
	::printf("debug ---> %s\n", st);
}



int main(int argc, char *argv[]) {
    ProducerConsumer::run();
	//test_reverse_string();

    return 0;
}
