package com.symphony.simpleserver.smb.api;

import com.fasterxml.jackson.annotation.JsonGetter;
import com.fasterxml.jackson.annotation.JsonSetter;
import java.util.List;

/**
 * The video description that SMB accepts. It has a few contraints because of how SMB operatoes as SFU
 * <ul>
 *  <li> All video tracks must support the same codecs, as the SFU does not transcode
 *  <li> Each video track is identified by an mslabel and all ssrcs within the track must have that label
 * <li> All video tracks must support the same RTP header extensions, or ignore them.
 * <li> All RTP header extentions must use the the same id map across all tracks
 * </ul>
 */
public class SmbVideo
{
    public List<SmbVideoTrack> tracks;
    @JsonGetter("streams") public List<SmbVideoTrack> getTracks() { return tracks; }
    @JsonSetter("streams") public void setTracks(List<SmbVideoTrack> tracks) { this.tracks = tracks; }

    public List<SmbPayloadType> payloadTypes;

    @JsonGetter("payload-types") public List<SmbPayloadType> getPayloadTypes() { return payloadTypes; }

    @JsonSetter("payload-types") public void setPayloadTypes(List<SmbPayloadType> payloadTypes)
    {
        this.payloadTypes = payloadTypes;
    }

    public List<SmbRtpHeaderExtension> rtpHeaderExtensions;

    @JsonGetter("rtp-hdrexts") public List<SmbRtpHeaderExtension> getRtpHeaderExtensions()
    {
        return rtpHeaderExtensions;
    }

    @JsonSetter("rtp-hdrexts") public void setRtpHeaderExtensions(List<SmbRtpHeaderExtension> rtpHeaderExtensions)
    {
        this.rtpHeaderExtensions = rtpHeaderExtensions;
    }
}
