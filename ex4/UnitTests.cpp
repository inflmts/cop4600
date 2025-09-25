#include <algorithm>
#include <limits.h>
#include "gtest/gtest.h"
#include "BackEnd.h"

#define TEST_INIT() \
	std::vector<Flight*> flights; \
	std::vector<Booking> expected; \
	std::vector<Flight*> tmp; \
	std::vector<Booking> result
#define FLIGHT(...) FLIGHT_INDIRECT(__VA_ARGS__, 20, _)
#define FLIGHT_INDIRECT(src, dest, time, capacity, ...) \
	Flight flight_ ## time (capacity, time, src, dest); \
	flights.push_back(&flight_ ## time)
#define BOOKING(...) BOOKING_INDIRECT(__VA_ARGS__, 2, 1, _)
#define BOOKING_INDIRECT(n1, n2, c, ...) \
	tmp = { BOOKING_PARAMS ## c (n1, n2) }; \
	expected.push_back(tmp)
#define BOOKING_PARAMS1(n, _) { &flight_ ## n }
#define BOOKING_PARAMS2(n1, n2) { &flight_ ## n1, &flight_ ## n2 }
#define TEST_QUERY(src, dest) \
	result = flightBookingQuery(flights, src, dest); \
	sanity_check_query(src, dest, flights, result); \
	sanity_check_query(src, dest, flights, expected); \
	ASSERT_EQ(result, expected)

/*
 *   1. A query returns all available flight bookings from a source airport to
 *	a destination airport.
 *
 *   2. A query will not return any flight booking that includes a full flight.
 *
 *   3. When a flight booking is purchased, the capacities of all flights in
 *	the booking should be decremented.
 *
 *   4. A booking either contains a single direct flight from the source
 *	airport to the destination airport, or two flights: one from the source
 *	airport to a connection airport, and a second from the connection
 *	airport to the destination airport. The second flight in a connection
 *	must depart at least 2 hours after the first flight’s departure.
 *
 *   5. A query will only return bookings with connections when there are no
 *	direct flights from the source airport to the destination airport.
 *
 *   6. If multiple flight bookings are returned by the query, bookings should
 *	be ordered by departure time, with the earliest flight first. In the
 *	case that a booking is a connection, it is ordered solely based on the
 *	departure time of the earlier flight.
 */

static bool is_one_or_two(int n)
{
	return n == 1 || n == 2;
}

static void sanity_check_query(Airport& src, Airport& dest,
		std::vector<Flight*>& flights, std::vector<Booking>& result)
{
	int prev_time = -1;
	int common_size = 0;
	for (Booking& booking : result) {
		std::vector<Flight*> booking_flights = booking.getFlights();
		// [2] No full flights
		for (Flight *flight : booking_flights) {
			int capacity = flight->getCapacity();
			ASSERT_GT(capacity, 0);
		}
		// [4] Single direct flight or connection between two flights
		ASSERT_PRED1(is_one_or_two, booking_flights.size());
		Airport *airport = &src;
		for (Flight *flight : booking_flights) {
			ASSERT_EQ(&flight->getSource(), airport);
			airport = &flight->getDestination();
		}
		ASSERT_EQ(airport, &dest);
		// [5] Bookings are either all direct or all connections
		if (!common_size) {
			common_size = booking_flights.size();
		} else {
			ASSERT_EQ(booking_flights.size(), common_size);
		}
		// [6] Ordered by departure time
		int cur_time = booking_flights[0]->getTakeoffHour();
		ASSERT_GT(cur_time, prev_time);
		prev_time = cur_time;
	}
}

static Airport BOS("BOS");
static Airport DEN("DEN");
static Airport FLL("FLL");
static Airport GNV("GNV");
static Airport LAS("LAS");
static Airport MCO("MCO");
static Airport MIA("MIA");
static Airport SJU("SJU");

/*
 * All tests will check for correct ordering.
 *
 * The specification does not define the output ordering if multiple matching
 * bookings start at the same time. This will not be tested.
 */

TEST(Query, ReturnsAll)
{
	TEST_INIT();
	FLIGHT(FLL, GNV, 10);
	FLIGHT(FLL, BOS, 2);
	FLIGHT(FLL, GNV, 4);
	FLIGHT(DEN, GNV, 8);
	FLIGHT(FLL, GNV, 6);
	BOOKING(4);
	BOOKING(6);
	BOOKING(10);
	TEST_QUERY(FLL, GNV);
	purchaseBooking(result[0]);
	ASSERT_EQ(flight_10.getCapacity(), 19);
	ASSERT_EQ(flight_2.getCapacity(), 20);
	ASSERT_EQ(flight_4.getCapacity(), 20);
	ASSERT_EQ(flight_8.getCapacity(), 20);
	ASSERT_EQ(flight_6.getCapacity(), 20);
}

TEST(Query, ReturnsNoFullFlights)
{
	TEST_INIT();
	FLIGHT(FLL, GNV, 10);
	FLIGHT(FLL, BOS, 2);
	FLIGHT(FLL, GNV, 4, 0);
	FLIGHT(FLL, GNV, 8);
	FLIGHT(FLL, GNV, 6);
	BOOKING(6);
	BOOKING(8);
	BOOKING(10);
	TEST_QUERY(FLL, GNV);
}

TEST(Query, ResolvesConnections)
{
	TEST_INIT();
	// This triplet should not be returned.
	FLIGHT(FLL, BOS, 21);
	FLIGHT(BOS, LAS, 26);
	FLIGHT(LAS, GNV, 28);
	FLIGHT(MIA, GNV, 9);
	FLIGHT(FLL, MIA, 7);
	FLIGHT(FLL, MCO, 3);
	FLIGHT(MCO, GNV, 6);
	FLIGHT(SJU, GNV, 5, 1);
	FLIGHT(FLL, SJU, 1);
	BOOKING(1, 5);
	BOOKING(3, 6);
	BOOKING(7, 9);
	TEST_QUERY(FLL, GNV);
	purchaseBooking(result[0]);
	ASSERT_EQ(flight_21.getCapacity(), 20);
	ASSERT_EQ(flight_26.getCapacity(), 20);
	ASSERT_EQ(flight_28.getCapacity(), 20);
	ASSERT_EQ(flight_9 .getCapacity(), 20);
	ASSERT_EQ(flight_7 .getCapacity(), 20);
	ASSERT_EQ(flight_3 .getCapacity(), 20);
	ASSERT_EQ(flight_6 .getCapacity(), 20);
	ASSERT_EQ(flight_5 .getCapacity(), 0);
	ASSERT_EQ(flight_1 .getCapacity(), 19);
	expected.clear();
	BOOKING(3, 6);
	BOOKING(7, 9);
	TEST_QUERY(FLL, GNV);
}

TEST(Query, PrioritizesDirectFlights)
{
	TEST_INIT();
	FLIGHT(MIA, GNV, 9);
	FLIGHT(FLL, MIA, 7);
	FLIGHT(FLL, GNV, 3);
	FLIGHT(MCO, GNV, 6);
	FLIGHT(FLL, GNV, 5);
	FLIGHT(FLL, GNV, 1);
	BOOKING(1);
	BOOKING(3);
	BOOKING(5);
	TEST_QUERY(FLL, GNV);
}

// vim:noet:sw=8:sts=8
