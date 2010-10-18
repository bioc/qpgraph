local({
    snowlib <- Sys.getenv("R_SNOW_LIB")
    outfile <- Sys.getenv("R_SNOW_OUTFILE")

    args <- commandArgs()
    pos <- match("--args", args)
    args <- args[-(1 : pos)]
    for (a in args) {
        pos <- regexpr("=", a)
        name <- substr(a, 1, pos - 1)
        value <- substr(a,pos + 1, nchar(a))
        switch(name,
               SNOWLIB = snowlib <- value,
               OUT = outfile <- value)
    }

    .libPaths(c(snowlib, .libPaths()))
    library(methods) ## because Rscript as of R 2.7.0 doesn't load methods
    library(rpvm, warn.conflicts=FALSE)
    library(snow, warn.conflicts=FALSE)

    sinkWorkerOutput(outfile)
    cat("starting PVM worker\n")
    slaveLoop(makePVMmaster())
    .PVM.exit()
})
