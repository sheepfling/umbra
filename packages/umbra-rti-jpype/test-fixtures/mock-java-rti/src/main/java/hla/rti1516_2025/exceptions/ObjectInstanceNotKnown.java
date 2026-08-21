package hla.rti1516_2025.exceptions;

/** Test-fixture declaration for the standard missing object-instance error. */
public class ObjectInstanceNotKnown extends RTIexception {
   public ObjectInstanceNotKnown(String message) {
      super(message);
   }
}
