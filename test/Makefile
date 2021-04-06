all:
	python3 test_semaphore.py
	python3 test_mutex.py
	python3 test_queue.py
	python3 test_fifo.py
	#python3 test_librertos.py
	python3 test_event.py
	python3 test_list.py

	./unit_test_c_with_python/py_loaded_gcovr.sh --context 0
	./unit_test_c_with_python/py_loaded_gcovr.sh --context 0 --branch

clean:
	rm -fr __pycache__/ *.pyc *_.c *.o *.so *.gcda *.gcno
