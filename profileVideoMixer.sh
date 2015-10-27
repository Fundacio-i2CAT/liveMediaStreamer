# profileVideoMixer <Base input RTSP URI> <Max channels>
# A testvideomixer will be instantiated connecting to the <Max channels> URI starting at
# <Base input URI> and incrementing the port by 2 for each one.

date > profileVideoMixer.log
echo $0 $1 $2 >> profileVideoMixer.log

date >> profileVideoMixer.stats
echo $0 $1 $2 >> profileVideoMixer.stats
echo "Selected Out Bitrate, Selected Channels, In Avg. Delay (us), Out Avg. Delay (us), Avg. Delay (us), In Losses, Out Losses, Losses, In Bitrate (kbs), In Pkt Losses, Out Bitrate (kbs), Out Pkt Losses, CPU%" >> profileVideoMixer.stats

baseport=`echo $1 | sed -e "s/.*:\([0-9]*\).*/\1/g"`
for bitrate in 1000 #2000 4000 8000
do
    commandline="./testvideomix -ow 1920 -oh 1080 -ob "$bitrate
    commandline+=" -oaddr 127.0.0.1 -oport 8650 -timeout 30 -statsfile profileVideoMixer.stats"
    port=$baseport
    for channels in `seq 1 1 $2`
    do
        commandline+=" -ri "`echo $1 | sed -e "s/:$baseport/:$port/g"`
        echo $commandline
        let port=$port+2
        echo $commandline >> profileVideoMixer.log
        $commandline >> profileVideoMixer.log 2>&1
    done
done
