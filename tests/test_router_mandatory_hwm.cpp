/*
    Copyright (c) 2007-2014 Contributors as noted in the AUTHORS file

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include "testutil.hpp"
#include <unistd.h>

#define DEBUG 0

int main (void)
{
    int rc;
    if (DEBUG) fprintf(stderr, "Staring router mandatory HWM test ...\n");
    setup_test_environment();
    void *ctx = zmq_ctx_new ();
    assert (ctx);
    void *router = zmq_socket (ctx, ZMQ_ROUTER);
    assert (router);

    // Configure router socket to mandatory routing and set HWM and linger
    int mandatory = 1;
    rc = zmq_setsockopt (router, ZMQ_ROUTER_MANDATORY, &mandatory, sizeof (mandatory));
    assert (rc == 0);
    int sndhwm = 1;
    rc = zmq_setsockopt (router, ZMQ_SNDHWM, &sndhwm, sizeof (sndhwm));
    assert (rc == 0);
    int linger = 1;
    rc = zmq_setsockopt (router, ZMQ_LINGER, &linger, sizeof (linger));
    assert (rc == 0);

    rc = zmq_bind (router, "tcp://127.0.0.1:5560");
    assert (rc == 0);

    //  Create dealer called "X" and connect it to our router, configure HWM
    void *dealer = zmq_socket (ctx, ZMQ_DEALER);
    assert (dealer);
    rc = zmq_setsockopt (dealer, ZMQ_IDENTITY, "X", 1);
    assert (rc == 0);
    int rcvhwm = 1;
    rc = zmq_setsockopt (dealer, ZMQ_RCVHWM, &rcvhwm, sizeof (rcvhwm));
    assert (rc == 0);

    rc = zmq_connect (dealer, "tcp://127.0.0.1:5560");
    assert (rc == 0);

    //  Get message from dealer to know when connection is ready
    char buffer [255];
    rc = zmq_send (dealer, "Hello", 5, 0);
    assert (rc == 5);
    rc = zmq_recv (router, buffer, 255, 0);
    assert (rc == 1);
    assert (buffer [0] ==  'X');

    int i;
    const int BUF_SIZE = 65536;
    char buf[BUF_SIZE];
    // Send first batch of messages
    for(i = 0; i < 100000; ++i) {
	if (DEBUG) fprintf(stderr, "Sending message %d ...\n", i);
	rc = zmq_send (router, "X", 1, ZMQ_DONTWAIT | ZMQ_SNDMORE);
	if (rc == -1 && zmq_errno() == EAGAIN) break;
	assert (rc == 1);
	rc = zmq_send (router, buf, BUF_SIZE, ZMQ_DONTWAIT);
	assert (rc == BUF_SIZE);
    }
    // This should fail after one message but kernel buffering could
    // skew results
    assert (i < 10);
    sleep(1);
    // Send second batch of messages
    for(; i < 100000; ++i) {
	if (DEBUG) fprintf(stderr, "Sending message %d (part 2) ...\n", i);
	rc = zmq_send (router, "X", 1, ZMQ_DONTWAIT | ZMQ_SNDMORE);
	if (rc == -1 && zmq_errno() == EAGAIN) break;
	assert (rc == 1);
	rc = zmq_send (router, buf, BUF_SIZE, ZMQ_DONTWAIT);
	assert (rc == BUF_SIZE);
    }
    // This should fail after two messages but kernel buffering could
    // skew results
    assert (i < 20);

    if (DEBUG) fprintf(stderr, "Done sending messages.\n");

    rc = zmq_close (router);
    assert (rc == 0);

    rc = zmq_close (dealer);
    assert (rc == 0);

    rc = zmq_ctx_term (ctx);
    assert (rc == 0);

    return 0 ;
}
