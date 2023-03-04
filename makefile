server: main.cpp ./threadpool/threadpool.h ./http/http_conn.cpp ./http/http_conn.h ./lock/locker.h ./timer/list_time.h ./timer/list_time.cpp
	g++ -o server main.cpp ./threadpool/threadpool.h ./http/http_conn.cpp ./http/http_conn.h ./lock/locker.h ./timer/list_time.h ./timer/list_time.cpp -lpthread 

clean:
	rm  -r server