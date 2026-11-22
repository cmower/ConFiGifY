package main

import (
	"fmt"
	"log"
	"os"

	"github.com/cmower/ConFiGifY/src/go"
)

func main() {
	if len(os.Args) != 2 {
		log.Fatal("expected a configuration path")
	}
	cfg, err := cfgy.Load(os.Args[1])
	if err != nil {
		log.Fatal(err)
	}
	samples, hasSamples := cfgy.Lookup(cfg, "recording", "samples")
	intervalMS, hasInterval := cfgy.Lookup(cfg, "recording", "interval_ms")
	if !hasSamples || !hasInterval {
		log.Fatal("expected recording.samples and recording.interval_ms")
	}
	fmt.Println(samples)
	fmt.Println(intervalMS)
}
