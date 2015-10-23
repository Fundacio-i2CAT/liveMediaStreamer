# profileVideoMixer <Base input RTSP URI> <Max channels>
# A testvideomixer will be instantiated connecting to the <Max channels> URI starting at
# <Base input URI> and incrementing the port by 2 for each one.

date > profileVideoMixer.log
echo $0 $1 $2 >> profileVideoMixer.log

date >> profileVideoMixer.stats
echo $0 $1 $2 >> profileVideoMixer.stats

baseport=`echo $1 | sed -e "s/.*:\([0-9]*\).*/\1/g"`
for bitrate in 1000 2000 4000 8000
do
    commandline="./testvideomix -ow 1920 -oh 1080 -ob "$bitrate
    commandline+=" -oaddr 127.0.0.1 -oport 8650 -timeout 10 -statsfile profileVideoMixer.stats"
    port=$baseport
    for channels in `seq 1 1 $2`
    do
        commandline+=" -ri "`echo $1 | sed -e "s/:$baseport/:$port/g"`
        echo $commandline
        let port=$port+2
        echo $commandline >> profileVideoMixer.log
        /usr/bin/time -f "%P" -o profileVideoMixer.stats -a $commandline >> profileVideoMixer.log 2> /dev/null
    done
done
