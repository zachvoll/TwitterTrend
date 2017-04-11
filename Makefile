# CSci4061 Assignment 3
# name: Zachary Vollen, Yadu Kiran

twitterTrend: 
	gcc -pthread -o twitterTrend twitterTrend.c

clean:
	rm -f *.o
	rm -f twitterTrend
	rm -f *.result
	
cr: 
	rm -f *.result

#used to run test cases
rt1:
	./twitterTrend client1.in 1

rt2:
	./twitterTrend client2.in 1

rt3:
	./twitterTrend client2.in 2
	
rt4:
	./twitterTrend client2.in 3

rt5:
	./twitterTrend client3.in 2
	
rt6:
	./twitterTrend client4.in 3
