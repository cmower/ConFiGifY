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
	count, hasCount := cfgy.Lookup(cfg, "c", "myel1")
	gain, hasGain := cfg["myfloat2"]
	if !hasCount || !hasGain {
		log.Fatal("expected c.myel1 and myfloat2")
	}
	fmt.Println(count)
	fmt.Println(gain)
}
