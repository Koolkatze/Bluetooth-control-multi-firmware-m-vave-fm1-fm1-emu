package com.koolkatze.fm1offline;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.Arrays;
import java.util.Random;

/** JDK-only wire/package tests of a real physical FM-1 USB MIDI updater.
 * No hardware is written by this test; no Android dependencies needed. */
public final class Fm1OtaProtocolTest {
    static void check(boolean condition, String what) {
        if (!condition) throw new AssertionError(what);
    }

    static byte[] identity(String product) {
        byte[] d = new byte[34];
        d[0] = 0; d[1] = 0x59; d[2] = 0x11;
        byte[] raw = product.getBytes(StandardCharsets.ISO_8859_1);
        System.arraycopy(raw, 0, d, 6, Math.min(27, raw.length));
        return Fm1OtaProtocol.wrap(d);
    }

    static byte[] request(long address, int length, int flags) {
        byte[] d = new byte[15];
        d[0] = 0; d[1] = 0x59; d[2] = 0x30;
        d[3] = 8; d[4] = 0; d[5] = 0; d[6] = (byte)flags;
        for (int i = 0; i < 4; ++i) d[7 + i] = (byte)(address >>> (8 * i));
        for (int i = 0; i < 3; ++i) d[11 + i] = (byte)(length >>> (8 * i));
        int sum = 0;
        for (int i = 6; i < 14; ++i) sum += d[i] & 255;
        d[14] = (byte)~sum;
        return Fm1OtaProtocol.wrap(d);
    }

    static byte[] packageFile(String product, boolean withLoader) {
        byte[] data = new byte[2048];
        Arrays.fill(data, (byte)0x5a);
        for (int i = 0; i < 20; ++i) data[i * 48 + 47] = 0x7d;
        for (int i = 0; i < product.length(); ++i)
            data[i * 48 + 47] = (byte)(product.charAt(i) + i + 1);
        if (withLoader)
            System.arraycopy(Fm1OtaProtocol.LOADER_MARK, 0, data, 1200,
                    Fm1OtaProtocol.LOADER_MARK.length);
        return data;
    }

    public static void main(String[] args) throws Exception {
        Random random = new Random(4567);
        for (int n : new int[]{0, 1, 2, 3, 7, 8, 14, 128, 512}) {
            byte[] original = new byte[n];
            random.nextBytes(original);
            byte[] restored = Fm1OtaProtocol.unpack7(Fm1OtaProtocol.pack7(original));
            check(Arrays.equals(original, restored), "7-bit pack roundtrip " + n);
        }
        byte[] sample = {(byte)0xe1, 0, 0x7f, (byte)0xff, 0x20};
        check(Arrays.equals(Fm1OtaProtocol.decode(Fm1OtaProtocol.wrap(sample)), sample),
                "SysEx wrapping roundtrip");
        byte[] r = request(0x1234L, 512, 0x7b);
        Fm1OtaProtocol.ReadRequest parsed = Fm1OtaProtocol.readRequest(r);
        check(parsed != null && parsed.address == 0x1234L &&
                parsed.length == 512 && parsed.flags == 0x7b, "M-UPGRADE block parsing");
        Fm1OtaProtocol.ReadRequest finish =
                Fm1OtaProtocol.readRequest(request(Fm1OtaProtocol.FINISH_WRITE, 0, 0));
        check(finish != null && finish.address == Fm1OtaProtocol.FINISH_WRITE,
                "M-UPGRADE write finished sentinel");
        byte[] bad = Arrays.copyOf(r, r.length);
        bad[bad.length - 3] ^= 1; // corrupt payload but keep valid end marker
        check(Fm1OtaProtocol.readRequest(bad) == null, "Corrupt SysEx rejected");
        byte[] image = new byte[512];
        random.nextBytes(image);
        byte[] reply = Fm1OtaProtocol.response(0x1234L, image, 0, 512, 0x7b);
        byte[] decoded = Fm1OtaProtocol.decode(reply);
        check(decoded.length == 527 && decoded[2] == 0x30, "Response layout");
        check((decoded[3] & 255) == 8 && (decoded[4] & 255) == 2, "Response length field");
        check(Arrays.equals(Arrays.copyOfRange(decoded, 14, 526), image), "Reply contains requested bytes");
        int checksum = 0;
        for (int i = 6; i < decoded.length; i++) checksum += decoded[i] & 255;
        check((checksum & 255) == 255, "Response checksum");
        byte[] framed = Fm1OtaProtocol.frame(49, new byte[]{1,2,3});
        check(Arrays.equals(new byte[]{1,2,3},
                Fm1OtaProtocol.queryPayload(framed, 49)), "Open firmware query response");
        check(Fm1OtaProtocol.queryPayload(framed, 50) == null,
                "Wrong query response command ignored");

        Fm1OtaProtocol.Identity ctl = Fm1OtaProtocol.identity(identity("FM-1_6000100"));
        check(ctl != null && ctl.model.equals("FM-1") && !ctl.loader && !ctl.open,
                "Physical FM-1 identification");
        Fm1OtaProtocol.Identity loader = Fm1OtaProtocol.identity(identity("ota-FM-1_6000100"));
        check(loader != null && loader.loader, "Physical updater loader identification");
        Fm1OtaProtocol.Identity open =
                Fm1OtaProtocol.identity(identity("FM1_OFFICIAL_OPEN"));
        check(open != null && open.open, "M-VAVE Open firmware identification");
        check(Fm1OtaProtocol.identity(identity("SMK-37_016")) == null,
                "Never install FM-1 firmware into SMK-37");

        byte[] pkg = packageFile("FM-1_6000100", true);
        Fm1OtaProtocol.Package parsedPkg = Fm1OtaProtocol.readPackage(pkg);
        check(parsedPkg.product.equals("FM-1_6000100"), "Package marker identity");
        check(parsedPkg.image.length == pkg.length - 20, "20 marker bytes removed");
        check(Arrays.equals(parsedPkg.digest, MessageDigest.getInstance("SHA-256").digest(pkg)),
                "Physical firmware checksum");
        check(Arrays.equals(
                Arrays.copyOfRange(parsedPkg.image, 0, 47),
                Arrays.copyOfRange(pkg, 0, 47)), "Logical image marker stripping");
        try {
            Fm1OtaProtocol.readPackage(packageFile("SMK-37_016", true));
            throw new AssertionError("SMK-37 package accepted");
        } catch (IllegalArgumentException good) { /* expected */ }
        try {
            Fm1OtaProtocol.readPackage(packageFile("FM-1_6000100", false));
            throw new AssertionError("Custom .fwsc without loader accepted");
        } catch (IllegalArgumentException good) { /* expected */ }
        check(Fm1OtaProtocol.readPackage(packageFile("FM-1_016", false)).stock,
                "Stock recovery firmware allowed");
        System.out.println("FM-1 physical USB OTA protocol: all tests passed");
    }
}
