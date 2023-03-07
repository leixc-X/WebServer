server: main.cpp ./threadpool/threadpool.h ./http/http_conn.cpp ./http/http_conn.h ./lock/locker.h ./timer/lst_timer.h ./log/log.cpp ./log/log.h ./log/block_queue.h
	g++ -o server main.cpp ./threadpool/threadpool.h ./http/http_conn.cpp ./http/http_conn.h ./lock/locker.h ./timer/lst_timer.h ./log/log.cpp ./log/log.h ./log/block_queue.h -lpthread  

clean:
	rm  -r server