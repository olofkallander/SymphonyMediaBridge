

export function tracksOf(sdp: string): string[]
{
    var lines = sdp.split('\n');
    var tracks: Array<string> = [];

    var intrack = false;
    var track = "";
    for (var l of lines)
    {
        if (!intrack && l.startsWith("m="))
        {
            track = l;
            intrack = true;
        }
        else if (intrack && l.startsWith("m="))
        {
            tracks.push(track);
            track = l;
        }
        else if (intrack)
        {
            track += "\n" + l;
        }
    }
    if (track.length > 0)
    {
        tracks.push(track);
    }
    return tracks;
}

function getCodecsOfMediaLine(mline: string): Set<number>
{
    let codecs = new Set<number>;

    if (!mline.startsWith("m="))
    {
        return codecs;
    }

    let regexp = new RegExp("m=\\S+\\s+(\\d+)\\s+(\\S+)\\s+([\\s\\d]+)", "gm");
    let match = regexp.exec(mline);
    if (match)
    {
        let codecsList = match[3].split(' ');
        for (var codec of codecsList)
        {
            codecs.add(Number.parseInt(codec));
        }
    }

    return codecs;
}

export function getCodecs(sdp: string, modality: string): Set<number>
{
    let codecs = new Set<number>;
    for (var track of tracksOf(sdp))
    {
        if (!track.startsWith("m=" + modality))
        {
            continue;
        }

        const lines = track.split('\n');
        let audioLine = lines[0];
        let moreCodecs = getCodecsOfMediaLine(audioLine);
        for (var v of moreCodecs)
        {
            codecs.add(v);
        }
    }
    return codecs;
}

export function getRtpExtensionsForModality(sdp: string, modality: string): Set<string>
{
    var extIds = new Set<string>();
    for (var track of tracksOf(sdp))
    {
        if (track.startsWith("m=" + modality))
        {
            return getRtpExtensions(track);
        }
    }

    return extIds;
}

export function getRtpExtensions(track: string): Set<string>
{
    var extIds = new Set<string>();
    let lines = track.split('\n');

    for (var line of lines)
    {
        let regexp = new RegExp("^a=extmap:(\\d+)\\s+(\\S+)", "gm");
        let match = regexp.exec(line.trim());
        //'a=extmap:3 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time'
        if (match)
        {
            extIds.add(match[2]);
        }
    }
    return extIds;
}

function getIntersection<T>(setA: Set<T>, setB: Set<T>): Set<T>
{
    return new Set(Array.from(setA).filter(element => setB.has(element)));
}

function notInB<T>(setA: Set<T>, setB: Set<T>): Set<T>
{
    return new Set(Array.from(setA).filter(element => !setB.has(element)));
}

function removeLinesContainingPattern(text: string, pattern: string): string
{
    const regex = new RegExp(`^.*${pattern}.*\\n`, 'gm');
    text = text.replace(regex, '');

    const regex2 = new RegExp(`\\n.*${pattern}.*`, 'gm');
    return text.replace(regex2, '');
}

function stripCodecsFromTrack(track: string, codecs: Set<number>): string
{
    let newTrack: string;

    const lines = track.split('\n');
    let offeredCodecs = getCodecsOfMediaLine(lines[0]);
    let unwantedCodecs = notInB(offeredCodecs, codecs);
    let newMediaLine = "";
    let regexp = new RegExp("(m=(\\S+)\\s+\\d+\\s+\\S+)\\s+[\\s\\d]+$", "gm");
    let match = regexp.exec(track);
    if (match)
    {
        newMediaLine = match[1];
        for (var c of codecs)
        {
            newMediaLine += " " + c;
        }
        track = removeLinesContainingPattern(track, match[1]);
    }

    for (var c of unwantedCodecs)
    {
        track = removeLinesContainingPattern(track, "a=fmtp:" + c as string);
        track = removeLinesContainingPattern(track, "a=rtpmap:" + c as string);
        track = removeLinesContainingPattern(track, "a=rtcp-fb:" + c as string);
    }

    newTrack = newMediaLine + '\n' + track;
    return newTrack;
}

export function stripSdp(sdp: string, offerSdp: string): string
{
    sdp = sdp.replace(/\r\n/gm, '\n');
    let audioCodecs = getCodecs(offerSdp, "audio");
    let videoCodecs = getCodecs(offerSdp, "video");
    let audioRtpExtIds = getRtpExtensionsForModality(offerSdp, "audio");
    let videoRtpExtIds = getRtpExtensionsForModality(offerSdp, "video");

    let newSdp = sdp.substring(0, sdp.indexOf("m=") - 1);

    for (let track of tracksOf(sdp))
    {
        if (track.startsWith("m=video"))
        {
            track = stripCodecsFromTrack(track, videoCodecs);
            let unwantedExts = notInB(getRtpExtensions(track), videoRtpExtIds);
            for (var eid of unwantedExts)
            {
                track = removeLinesContainingPattern(track, eid);
            }
            newSdp += '\n' + track;
        }
        else if (track.startsWith("m=audio"))
        {
            track = stripCodecsFromTrack(track, audioCodecs);
            let unwantedExts = notInB(getRtpExtensions(track), audioRtpExtIds);
            for (var eid of unwantedExts)
            {
                track = removeLinesContainingPattern(track, eid);
            }
            newSdp += '\n' + track;
        }
        else
        {
            newSdp += '\n' + track;
        }
    }

    return newSdp;
}

export function addAttributeToTrack(sdp: string, modality: string, trackIndex: number, attr: string)
{
    for (let track of tracksOf(sdp))
    {
        if (!track.startsWith("m=" + modality))
        {
            continue;
        }

        if (trackIndex > 0)
        {
            trackIndex--;
            continue;
        }

        let newTrack = track + '\n' + attr;
        return sdp.replace(track, newTrack);
    }

    return sdp;
}