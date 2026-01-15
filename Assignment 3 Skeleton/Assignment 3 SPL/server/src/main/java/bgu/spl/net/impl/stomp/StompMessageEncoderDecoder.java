package bgu.spl.net.impl.stomp;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;

import bgu.spl.net.api.MessageEncoderDecoder;

public class StompMessageEncoderDecoder implements MessageEncoderDecoder<String>{
    
    private byte[] bytes = new byte[1 << 10]; // 1KB buffer
    private int len = 0;
    
    @Override
    public String decodeNextByte(byte nextByte) {
        if (nextByte == '\0') {// message is complete
            String result = new String(bytes, 0, len, StandardCharsets.UTF_8);
            len = 0;
            return result;
        }

        // else, keep accumulating bytes
        if (len >= bytes.length) {
            bytes = Arrays.copyOf(bytes, len * 2);// increase buffer size
        }
        bytes[len++] = nextByte;
        
        return null;
    }

    @Override
    public byte[] encode(String message) {
        return (message + "\0").getBytes(StandardCharsets.UTF_8);// \0 is added to indicate end of message for the decoder
    }

}
