package org.hla.rti.tck;

import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.RTIexception;

/** Checks the standard Java exception name for one pre-Connect service failure. */
final class ExceptionNameStringTck {
   private ExceptionNameStringTck() { }

   static void run(RtiFactory factory) throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      RTIexception observed = null;
      try {
         ambassador.getObjectClassHandle("HLAobjectRoot");
      } catch (RTIexception exception) {
         observed = exception;
      }
      JavaTckSupport.check(observed != null,
         "getObjectClassHandle before Connect did not throw an RTIexception");
      JavaTckSupport.check("NotConnected".equals(observed.name()),
         "pre-Connect getObjectClassHandle did not report the exact NotConnected name");
   }
}
