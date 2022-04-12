all: serverA serverB serverC serverM clientA clientB
.PHONY: all
serverA: serverA.cpp
	g++ -std=c++11 -o serverA serverA.cpp
serverB: serverB.cpp
	g++ -std=c++11 -o serverB serverB.cpp
serverC: serverC.cpp
	g++ -std=c++11 -o serverC serverC.cpp
serverM: serverM.cpp
	g++ -std=c++11 -o serverM serverM.cpp
clientA: clientA.cpp
	g++ -std=c++11 -o clientA clientA.cpp
clientB: clientB.cpp
	g++ -std=c++11 -o clientB clientB.cpp
