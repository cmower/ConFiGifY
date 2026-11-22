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
	fmt.Println(cfg["c"].(map[string]any)["myel1"])
	fmt.Println(cfg["myfloat2"])
}
