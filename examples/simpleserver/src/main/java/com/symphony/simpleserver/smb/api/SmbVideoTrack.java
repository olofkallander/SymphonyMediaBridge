package com.symphony.simpleserver.smb.api;

import java.util.List;

/**
 * A media stream track that contains one or several ssrc groups. Eg. simulcast track.
 * The id is the mslabel of the "track"
 */
public class SmbVideoTrack
{
    public static class SmbVideoSource
    {
        public long main;
        public long feedback;
    }

    public List<SmbVideoSource> sources;
    public String id;
    public String content;
}
