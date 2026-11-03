package com.koolkatze.fm1offline;

import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.Arrays;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * USB-MIDI Felucca / M-UPGRADE wire protocol, ported from fm1-base's
 * tools/fm1_install.py and web/fm1ota.js (MPL-2.0).
 * Copyright (c) 2026 Charles Vestal, Leo Kuroshita and contributors.
 * This class contains NO Android dependencies so the packet format and the
 * package guardrails are exercised by javac tests in GitHub Actions.
 */
final class Fm1OtaProtocol {
    static final byte[] HANDSHAKE = {(byte)0xf0,0,0x32,0x45,0,0,0,0x40,0x7f,(byte)0xf7};
    static final byte[] UPGRADE = {(byte)0xf0,0x22,0x24,0x35,0x7f,(byte)0xf7};
    static final long FINISH_CHECK = 0xe0000000L, FINISH_WRITE = 0xf0000000L;
    static final int MAX_PACKAGE = 15 * 1024 * 1024, MAX_REQUEST = 512;
    static final int BLOCKS = 20, BLOCK = 48, MARKER = 47;
    static final byte[] LOADER_MARK = "FELUCCA-LOADER-1".getBytes(StandardCharsets.US_ASCII);

    private Fm1OtaProtocol() {}

    static final class Package {
        final byte[] image;
        final byte[] digest;
        final String product;
        final boolean stock;
        Package(byte[] image, byte[] digest, String product, boolean stock) {
            this.image = image; this.digest = digest;
            this.product = product; this.stock = stock;
        }
    }
    static final class Identity {
        final String text, model;
        final boolean loader, open;
        Identity(String text, String model, boolean loader, boolean open) {
            this.text = text; this.model = model;
            this.loader = loader; this.open = open;
        }
    }
    static final class ReadRequest {
        final int flags, length;
        final long address;
        ReadRequest(int flags, long address, int length) {
            this.flags = flags; this.address = address; this.length = length;
        }
    }

    static byte[] pack7(byte[] data) {
        ByteArrayOutputStream out = new ByteArrayOutputStream(data.length * 8 / 7 + 2);
        int acc = 0, bits = 0;
        for (byte b : data) {
            acc |= (b & 255) << bits;
            bits += 8;
            while (bits >= 7) {
                out.write(acc & 127);
                acc >>>= 7;
                bits -= 7;
            }
        }
        if (bits != 0) out.write(acc & 127);
        return out.toByteArray();
    }

    static byte[] unpack7(byte[] data) {
        ByteArrayOutputStream out = new ByteArrayOutputStream(data.length * 7 / 8 + 1);
        int acc = 0, bits = 0;
        for (byte b : data) {
            if ((b & 128) != 0) return new byte[0];
            acc |= (b & 127) << bits;
            bits += 7;
            while (bits >= 8) {
                out.write(acc & 255);
                acc >>>= 8;
                bits -= 8;
            }
        }
        return out.toByteArray();
    }

    static byte[] wrap(byte[] payload) {
        byte[] packed = pack7(payload);
        byte[] result = new byte[packed.length + 2];
        result[0] = (byte)0xf0;
        System.arraycopy(packed, 0, result, 1, packed.length);
        result[result.length - 1] = (byte)0xf7;
        return result;
    }

    static byte[] decode(byte[] sysex) {
        if (sysex == null || sysex.length < 3
                || (sysex[0] & 255) != 0xf0
                || (sysex[sysex.length - 1] & 255) != 0xf7)
            return null;
        return unpack7(Arrays.copyOfRange(sysex, 1, sysex.length - 1));
    }

    static byte[] response(long address, byte[] image, int start, int len, int flags) {
        if (len < 0 || len > MAX_REQUEST || start < 0 || start > image.length
                || len > image.length - start)
            throw new IllegalArgumentException("Read request outside firmware image");
        byte[] data = new byte[15 + len];
        data[0] = 0; data[1] = 0x59; data[2] = 0x30;
        data[3] = (byte)((len + 8) & 255);
        data[4] = (byte)((len + 8) >>> 8);
        data[5] = 0;
        data[6] = (byte)flags;
        for (int i = 0; i < 4; ++i) data[7 + i] = (byte)(address >>> (8 * i));
        data[11] = (byte)len;
        data[12] = (byte)(len >>> 8);
        data[13] = 0;
        System.arraycopy(image, start, data, 14, len);
        int sum = 0;
        for (int i = 6; i < data.length - 1; ++i) sum += data[i] & 255;
        data[data.length - 1] = (byte)~sum;
        return wrap(data);
    }

    static ReadRequest readRequest(byte[] sysex) {
        byte[] d = decode(sysex);
        if (d == null || d.length != 15 || d[0] != 0 || d[1] != 0x59 || d[2] != 0x30)
            return null;
        int sum = 0;
        for (int i = 6; i < 14; ++i) sum += d[i] & 255;
        if (((~sum) & 255) != (d[14] & 255)) return null;
        long address = 0;
        for (int i = 0; i < 4; ++i) address |= ((long)(d[7 + i] & 255)) << (8 * i);
        int length = (d[11] & 255) | ((d[12] & 255) << 8) | ((d[13] & 255) << 16);
        return new ReadRequest(d[6] & 255, address, length);
    }

    static byte[] frame(int command, byte[] payload) {
        if (payload.length > 65535) throw new IllegalArgumentException("payload too large");
        byte[] raw = new byte[7 + payload.length];
        raw[0] = 0; raw[1] = 0x59; raw[2] = (byte)command;
        raw[3] = (byte)payload.length; raw[4] = (byte)(payload.length >>> 8);
        raw[5] = 0;
        System.arraycopy(payload, 0, raw, 6, payload.length);
        int sum = 0;
        for (byte b : payload) sum += b & 255;
        raw[raw.length - 1] = (byte)~sum;
        return wrap(raw);
    }

    static byte[] queryPayload(byte[] sysex, int command) {
        byte[] d = decode(sysex);
        if (d == null || d.length < 7 || d[0] != 0 || d[1] != 0x59 || (d[2] & 255) != command)
            return null;
        int n = (d[3] & 255) | ((d[4] & 255) << 8);
        if (n != d.length - 7) return null;
        int sum = 0;
        for (int i = 6; i < d.length; ++i) sum += d[i] & 255;
        return (sum & 255) == 255 ? Arrays.copyOfRange(d, 6, 6 + n) : null;
    }

    static Identity identity(byte[] sysex) {
        byte[] d = decode(sysex);
        if (d == null || d.length != 34 || d[0] != 0 || d[1] != 0x59 || d[2] != 0x11)
            return null;
        int end = 33;
        while (end > 6 && d[end - 1] == 0) end--;
        String product = new String(d, 6, end - 6, StandardCharsets.ISO_8859_1);
        if (product.matches("FM1_(OFFICIAL_OPEN|COMMUNITY_\\d+)"))
            return new Identity(product, "FM-1", false, true);
        if (product.matches("ota-FM1C_[a-fA-F0-9]+"))
            return new Identity(product, "FM-1", true, true);
        Matcher m = Pattern.compile("^([^_]+)_([0-9]+)$").matcher(product);
        if (!m.matches()) return null;
        boolean loader = m.group(1).startsWith("ota-");
        String model = m.group(1).replaceFirst("^ota-", "");
        if (!model.equals("FM-1")) return null; // never send an FM-1 firmware to SMK-37
        boolean open = product.matches("FM-1_80000\\d\\d");
        return new Identity(product, model, loader, open);
    }

    static boolean isStock(String product) {
        return product.matches("FM-1_0[0-9][0-9]") && !product.equals("FM-1_099");
    }

    static Package readPackage(byte[] raw) throws Exception {
        if (raw == null || raw.length < 1024 || raw.length > MAX_PACKAGE)
            throw new IllegalArgumentException("El archivo .fwsc tiene un tamaño no admitido");
        StringBuilder text = new StringBuilder();
        for (int i = 0; i < BLOCKS; ++i) {
            int marker = raw[i * BLOCK + MARKER] & 255;
            if (marker != 0x7d) text.append((char)((marker - i - 1) & 255));
        }
        String product = text.toString();
        if (!product.matches("FM-1_[0-9]{1,16}"))
            throw new IllegalArgumentException("Paquete no válido para FM-1: identidad " + product);
        boolean stock = isStock(product);
        boolean loader = contains(raw, LOADER_MARK);
        if (!stock && !loader)
            throw new IllegalArgumentException("Firmware personalizado sin cargador de actualización Felucca verificado. No se instalará en el dispositivo.");
        byte[] image = new byte[raw.length - BLOCKS];
        int offset = 0;
        for (int i = 0; i < BLOCKS; ++i) {
            System.arraycopy(raw, i * BLOCK, image, offset, MARKER);
            offset += MARKER;
        }
        System.arraycopy(raw, BLOCKS * BLOCK, image, offset, raw.length - BLOCKS * BLOCK);
        byte[] digest = MessageDigest.getInstance("SHA-256").digest(raw);
        return new Package(image, digest, product, stock);
    }

    private static boolean contains(byte[] source, byte[] marker) {
        outer: for (int i = 0; i + marker.length <= source.length; i++) {
            for (int j = 0; j < marker.length; j++)
                if (source[i + j] != marker[j]) continue outer;
            return true;
        }
        return false;
    }

    static String hex(byte[] digest) {
        char[] digits = "0123456789abcdef".toCharArray();
        char[] result = new char[digest.length * 2];
        for (int i = 0; i < digest.length; ++i) {
            result[2 * i] = digits[(digest[i] & 255) >>> 4];
            result[2 * i + 1] = digits[digest[i] & 15];
        }
        return new String(result);
    }
}
