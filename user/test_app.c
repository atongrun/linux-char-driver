#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEVICE_PATH "/dev/char0"
#define READ_BUF_SIZE 1024

int main(int argc, char *argv[])
{
	int fd;
	ssize_t ret;
	const char *msg = "hello from user space";
	char read_buf[READ_BUF_SIZE];

	if (argc > 1)
		msg = argv[1];

	fd = open(DEVICE_PATH, O_RDWR);
	if (fd < 0) {
		perror("open");
		return EXIT_FAILURE;
	}

	ret = write(fd, msg, strlen(msg));
	if (ret < 0) {
		perror("write");
		close(fd);
		return EXIT_FAILURE;
	}
	printf("Wrote %zd bytes: \"%s\"\n", ret, msg);

	if (lseek(fd, 0, SEEK_SET) < 0) {
		perror("lseek");
		close(fd);
		return EXIT_FAILURE;
	}

	memset(read_buf, 0, sizeof(read_buf));
	ret = read(fd, read_buf, sizeof(read_buf) - 1);
	if (ret < 0) {
		perror("read");
		close(fd);
		return EXIT_FAILURE;
	}
	read_buf[ret] = '\0';
	printf("Read %zd bytes: \"%s\"\n", ret, read_buf);

	close(fd);
	return EXIT_SUCCESS;
}
