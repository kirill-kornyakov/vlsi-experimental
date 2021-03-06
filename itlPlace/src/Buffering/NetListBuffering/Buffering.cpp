#include "Buffering.h"
#include "BufferInfo.h"
#include "NetInfo.h"
#include "TimingHelpers.h"

void BufferingAndReport(HDesign& design)
{
    {
        ConfigContext ctx(design.cfg.OpenContext("Buffering"));

        BufferInfo buf = BufferInfo::Create(design);
        //VanGinneken vg(design);

        WRITELINE("Buffering Parameters:");
        WRITELINE("Rb: %.20f kOhm", buf.Rb());
        WRITELINE("Cb: %.20f pF", buf.Cb());
        WRITELINE("Tb: %.20f ns", buf.Tb());
        WRITELINE("r:  %.20f kOhm/nm", design.RoutingLayers.Physics.RPerDist);
        WRITELINE("c:  %.20f pf/nm", design.RoutingLayers.Physics.LinearC);
        WRITELINE("Lbuf: %.20f", buf.Lbuf());
        WRITELINE("Dbuf: %.20f", buf.Dbuf());

        int buffers = 0;
        int vgbuffers = 0;

        string h1 = "fanout ",     b1 = "%6d ";
        string h2 = "vgbufs ",     b2 = "%6d ";
        string h3 = "kopt ",       b3 = "%4d ";
        string h4 = "koptd ",      b4 = "%5.3f ";
        string h5 = "   knaked ",  b5 = "%9.5f ";
        string h6 = "   T(kopt) ", b6 = "%10.8f ";
        string h7 = "  T(koptd) ", b7 = "%10.8f ";
        string hh = "   vgSlack ", bb = "%10.8f ";
        string h8 = "      T(0) ", b8 = "%10.8f ";
        string h9 = "  maxSlack ", b9 = "%10.8f ";
        string f1 = "      HPWL ", c1 = "%10.2f ";
        string f2 = "        WL ", c2 = "%10.2f ";
        string f3 = "        Rd ", c3 = "%10.8f ";
        string f4 = "        Cs ", c4 = "%10.8f ";
        string f5 = "OO ",         c5 = "%2d ";
        string f6 = "OM ",         c6 = "%2d ";
        string f7 = "OE ",         c7 = "%2d ";
        string ff = "      Name",  cc = "%10s";

        string header = h1 + h2 + h3 + h4 + h5 + h6 + h7 + h8 + hh + h9 + f1 + f2 + f3 + f4 + f5 + f6 + f7 + ff;
        string bodyer = b1 + b2 + b3 + b4 + b5 + b6 + b7 + b8 + bb + b9 + c1 + c2 + c4 + c4 + c5 + c6 + c7 + cc;
        WRITELINE(header.c_str());
        for(HNets::ActiveNetsEnumeratorW net = design.Nets.GetActiveNetsEnumeratorW(); net.MoveNext(); )
        {
            NetInfo n = NetInfo::Create(design, net, buf);
            buffers += n.KoptInt();

            //vg.__DriverResistance = n.Rd();
            int vgbufs = 0;//vg.InsertBuffers(net);
            vgbuffers += vgbufs;

            double bufferedDelay = TimingHelper(design).GetBufferedNetMaxDelay(net, n, buf);

            WRITELINE(bodyer.c_str()
                , n.Fanout() //h1
                , vgbufs //h2
                , n.KoptInt() //h3
                , n.Kopt() //h4
                , n.KoptNaked() //h5
                , n.KoptDelay() //h6
                , n.OptimalDelay() //h7
                , n.OttenDelay() //h8
                , bufferedDelay //hh
                , n.MaxRealDelay() //h9
                , n.HPWL() //f1
                , n.WL() //f2
                , n.Rd() //f3
                , n.Cs() //f4
                , n.IsOttenApplicable() //f5
                , n.IsModifiedOttenApplicable() //f6
                , n.IsOttenApplicableExact() //f7
                , n.Name().c_str() //ff
                );
            //if ((n.IsOttenApplicable() || n.IsModifiedOttenApplicable()))
            //{

            //WRITELINE("%3d %3d %.5f %.5f %.5f %.5f %.5f %.5f %d %.5f %d",
            //n.Fanout(), n.KoptInt(), n.Kopt(),
            //n.KoptDelay(), n.OptimalDelay(), n.OttenDelay(), n.MaxRealDelay(), n.L(), n.IsOttenApplicable(), n.X2opt(), nets);
            //}
        }

        WRITELINE("Inserted             %d buffers", buffers);
        WRITELINE("VanGinneken Inserted %d buffers", vgbuffers);
    }
}
