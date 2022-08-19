// Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
// 
// This file is part of Orion Context Broker.
//
// Orion Context Broker is free software: you can redistribute it and/or
// modify it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// Orion Context Broker is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
// General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
//
// For those usages not covered by this license please contact with
// iot_support at tid dot es

package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"net/http"
	"sync"
	"time"
)

var (
	mtx           sync.Mutex
	count, before int
)

var (
	delay         = flag.Duration("delay", 0, "delay in write response")
	statsInterval = flag.Duration("stats", 5*time.Second, "interval for writing stats")
	port          = flag.String("port", "8090", "port to listen to (8090 by default)")
	timeout       = flag.Duration("timeout", 30*time.Second, "timeout")
	close         = flag.Bool("close", false, "do not send Connection:close in response")
)

func main() {

	flag.Parse()

	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		defer r.Body.Close()
		time.Sleep(*delay)
		ioutil.ReadAll(r.Body)
		if *close {
			w.Header().Set("Connection", "close")
		}
		w.Write([]byte("OK"))
		mtx.Lock()
		count++
		mtx.Unlock()
	})
	go func() {
		for {
			time.Sleep((*statsInterval))
			mtx.Lock()
			fmt.Println("Received ", count, "Rate", float64(count-before)/float64(*statsInterval/(time.Second)), "r/s")
			before = count
			mtx.Unlock()
		}
	}()
	s := &http.Server{
		Addr:           ":" + *port,
		Handler:        http.DefaultServeMux,
		ReadTimeout:    *timeout,
		WriteTimeout:   *timeout,
		MaxHeaderBytes: 1 << 20,
	}
	fmt.Println(s.ListenAndServe())
}

