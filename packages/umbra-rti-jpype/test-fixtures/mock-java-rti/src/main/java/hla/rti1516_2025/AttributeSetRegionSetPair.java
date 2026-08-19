package hla.rti1516_2025;

/** Java-shaped attribute-to-region association used by the adapter fixture. */
public final class AttributeSetRegionSetPair {
  private final AttributeHandleSet attributes;
  private final RegionHandleSet regions;

  public AttributeSetRegionSetPair(AttributeHandleSet attributes, RegionHandleSet regions) {
    this.attributes = attributes;
    this.regions = regions;
  }

  public AttributeHandleSet getAttributes() {
    return attributes;
  }

  public RegionHandleSet getRegions() {
    return regions;
  }
}
