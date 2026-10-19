// Generated from the named Python workspace models. Do not edit.
// Contract SHA-256: 364a8d9ab95c1895df933f0d0f75565b8fad74ae43e6e40c7ff2174ec065b89a
"use strict";
exports.validateWorkspace = validate20;
const schema31 = {"$defs":{"AlertViewSettings":{"additionalProperties":false,"properties":{"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"AlertViewSettings","type":"object"},"CoordinationViewSettings":{"additionalProperties":false,"properties":{"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"phase":{"default":"all","maxLength":8192,"title":"Phase","type":"string"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"default":"all","maxLength":8192,"title":"Selected","type":"string"}},"title":"CoordinationViewSettings","type":"object"},"DeclarationViewSettings":{"additionalProperties":false,"properties":{"direction":{"default":"all","maxLength":8192,"title":"Direction","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"perspective":{"default":"all","maxLength":8192,"title":"Perspective","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"DeclarationViewSettings","type":"object"},"EventViewSettings":{"additionalProperties":false,"properties":{"family":{"default":"","maxLength":8192,"title":"Family","type":"string"},"federate":{"default":"","maxLength":8192,"title":"Federate","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"source":{"default":"","maxLength":8192,"title":"Source","type":"string"},"type":{"default":"","maxLength":8192,"title":"Type","type":"string"}},"title":"EventViewSettings","type":"object"},"FederationViewSettings":{"additionalProperties":false,"properties":{"distribution":{"default":"event_types","enum":["event_types","sources","families"],"title":"Distribution","type":"string"}},"title":"FederationViewSettings","type":"object"},"FomViewSettings":{"additionalProperties":false,"properties":{"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"model":{"default":"all","maxLength":8192,"title":"Model","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"FomViewSettings","type":"object"},"InteractionViewSettings":{"additionalProperties":false,"properties":{"order":{"default":"all","maxLength":8192,"title":"Order","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"InteractionViewSettings","type":"object"},"MomViewSettings":{"additionalProperties":false,"properties":{"counter":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Counter"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"service_status":{"default":"all","maxLength":8192,"title":"Service Status","type":"string"}},"title":"MomViewSettings","type":"object"},"ObjectChartChoice":{"additionalProperties":false,"properties":{"attribute":{"maxLength":8192,"minLength":1,"title":"Attribute","type":"string"},"object_key":{"maxLength":8192,"minLength":1,"title":"Object Key","type":"string"},"scope":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Scope"}},"required":["object_key","attribute"],"title":"ObjectChartChoice","type":"object"},"ObjectViewSettings":{"additionalProperties":false,"properties":{"scope":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Scope"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"default":"","maxLength":8192,"title":"Selected","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"ObjectViewSettings","type":"object"},"OwnershipViewSettings":{"additionalProperties":false,"properties":{"owner_state":{"default":"all","maxLength":8192,"title":"Owner State","type":"string"},"request_status":{"default":"all","maxLength":8192,"title":"Request Status","type":"string"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"}},"title":"OwnershipViewSettings","type":"object"},"RegionViewSettings":{"additionalProperties":false,"properties":{"evidence":{"default":"all","maxLength":8192,"title":"Evidence","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"RegionViewSettings","type":"object"},"TimeViewSettings":{"additionalProperties":false,"properties":{"grants_only":{"default":false,"title":"Grants Only","type":"boolean"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"}},"title":"TimeViewSettings","type":"object"},"TopologyViewSettings":{"additionalProperties":false,"properties":{"evidence":{"default":"all","maxLength":8192,"title":"Evidence","type":"string"},"focused":{"default":"all","maxLength":8192,"title":"Focused","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"source":{"default":"all","maxLength":8192,"title":"Source","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"TopologyViewSettings","type":"object"},"WorkspaceSettings":{"additionalProperties":false,"properties":{"alerts":{"$ref":"#/$defs/AlertViewSettings","default":{"search":"","status":"all"}},"declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"federate_declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"federate_events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"federate_time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}},"federation":{"$ref":"#/$defs/FederationViewSettings","default":{"distribution":"event_types"}},"fom":{"$ref":"#/$defs/FomViewSettings","default":{"kind":"all","model":"all","search":"","selected":null,"status":"all"}},"interactions":{"$ref":"#/$defs/InteractionViewSettings","default":{"order":"all","search":"","status":"all"}},"mom":{"$ref":"#/$defs/MomViewSettings","default":{"counter":null,"scope":"all","search":"","service_status":"all"}},"object_charts":{"default":[],"items":{"$ref":"#/$defs/ObjectChartChoice"},"maxItems":64,"title":"Object Charts","type":"array"},"objects":{"$ref":"#/$defs/ObjectViewSettings","default":{"scope":null,"search":"","selected":"","status":"all"}},"ownership":{"$ref":"#/$defs/OwnershipViewSettings","default":{"owner_state":"all","request_status":"all","scope":"all","search":""}},"persistence":{"$ref":"#/$defs/CoordinationViewSettings","default":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"}},"regions":{"$ref":"#/$defs/RegionViewSettings","default":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"}},"selected_federate":{"default":"","maxLength":8192,"title":"Selected Federate","type":"string"},"synchronization":{"$ref":"#/$defs/CoordinationViewSettings","default":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"}},"time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}},"topology":{"$ref":"#/$defs/TopologyViewSettings","default":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"}},"view":{"default":"federation","enum":["federation","federates","objects","declarations","topology","interactions","ownership","regions","mom","fom","time","synchronization","persistence","events","inspectors","recordings","alerts","actions","diagnostics"],"title":"View","type":"string"},"window_seconds":{"default":300,"enum":[30,300,900,3600],"title":"Window Seconds","type":"integer"}},"title":"WorkspaceSettings","type":"object"}},"$id":"urn:umbra:observer:workspace:v1","$schema":"https://json-schema.org/draft/2020-12/schema","additionalProperties":false,"properties":{"name":{"maxLength":128,"minLength":1,"pattern":"\\S","title":"Name","type":"string"},"settings":{"$ref":"#/$defs/WorkspaceSettings","default":{"alerts":{"search":"","status":"all"},"declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"events":{"family":"","federate":"","search":"","source":"","type":""},"federate_declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"federate_events":{"family":"","federate":"","search":"","source":"","type":""},"federate_time":{"grants_only":false,"selected":null},"federation":{"distribution":"event_types"},"fom":{"kind":"all","model":"all","search":"","selected":null,"status":"all"},"interactions":{"order":"all","search":"","status":"all"},"mom":{"counter":null,"scope":"all","search":"","service_status":"all"},"object_charts":[],"objects":{"scope":null,"search":"","selected":"","status":"all"},"ownership":{"owner_state":"all","request_status":"all","scope":"all","search":""},"persistence":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"regions":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"},"selected_federate":"","synchronization":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"time":{"grants_only":false,"selected":null},"topology":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"},"view":"federation","window_seconds":300}},"workspace_version":{"const":"umbra-observer-workspace-v1","title":"Workspace Version","type":"string"}},"required":["workspace_version","name"],"title":"WorkspaceDocument","type":"object"};
const func0 = Object.prototype.hasOwnProperty;
const func63 = require("ajv/dist/runtime/ucs2length").default;
const pattern4 = new RegExp("\\S", "u");
const schema32 = {"additionalProperties":false,"properties":{"alerts":{"$ref":"#/$defs/AlertViewSettings","default":{"search":"","status":"all"}},"declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"federate_declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"federate_events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"federate_time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}},"federation":{"$ref":"#/$defs/FederationViewSettings","default":{"distribution":"event_types"}},"fom":{"$ref":"#/$defs/FomViewSettings","default":{"kind":"all","model":"all","search":"","selected":null,"status":"all"}},"interactions":{"$ref":"#/$defs/InteractionViewSettings","default":{"order":"all","search":"","status":"all"}},"mom":{"$ref":"#/$defs/MomViewSettings","default":{"counter":null,"scope":"all","search":"","service_status":"all"}},"object_charts":{"default":[],"items":{"$ref":"#/$defs/ObjectChartChoice"},"maxItems":64,"title":"Object Charts","type":"array"},"objects":{"$ref":"#/$defs/ObjectViewSettings","default":{"scope":null,"search":"","selected":"","status":"all"}},"ownership":{"$ref":"#/$defs/OwnershipViewSettings","default":{"owner_state":"all","request_status":"all","scope":"all","search":""}},"persistence":{"$ref":"#/$defs/CoordinationViewSettings","default":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"}},"regions":{"$ref":"#/$defs/RegionViewSettings","default":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"}},"selected_federate":{"default":"","maxLength":8192,"title":"Selected Federate","type":"string"},"synchronization":{"$ref":"#/$defs/CoordinationViewSettings","default":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"}},"time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}},"topology":{"$ref":"#/$defs/TopologyViewSettings","default":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"}},"view":{"default":"federation","enum":["federation","federates","objects","declarations","topology","interactions","ownership","regions","mom","fom","time","synchronization","persistence","events","inspectors","recordings","alerts","actions","diagnostics"],"title":"View","type":"string"},"window_seconds":{"default":300,"enum":[30,300,900,3600],"title":"Window Seconds","type":"integer"}},"title":"WorkspaceSettings","type":"object"};
const schema33 = {"additionalProperties":false,"properties":{"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"AlertViewSettings","type":"object"};
const schema34 = {"additionalProperties":false,"properties":{"direction":{"default":"all","maxLength":8192,"title":"Direction","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"perspective":{"default":"all","maxLength":8192,"title":"Perspective","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"DeclarationViewSettings","type":"object"};
const schema35 = {"additionalProperties":false,"properties":{"family":{"default":"","maxLength":8192,"title":"Family","type":"string"},"federate":{"default":"","maxLength":8192,"title":"Federate","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"source":{"default":"","maxLength":8192,"title":"Source","type":"string"},"type":{"default":"","maxLength":8192,"title":"Type","type":"string"}},"title":"EventViewSettings","type":"object"};
const schema38 = {"additionalProperties":false,"properties":{"grants_only":{"default":false,"title":"Grants Only","type":"boolean"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"}},"title":"TimeViewSettings","type":"object"};
const schema39 = {"additionalProperties":false,"properties":{"distribution":{"default":"event_types","enum":["event_types","sources","families"],"title":"Distribution","type":"string"}},"title":"FederationViewSettings","type":"object"};
const schema40 = {"additionalProperties":false,"properties":{"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"model":{"default":"all","maxLength":8192,"title":"Model","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"FomViewSettings","type":"object"};
const schema41 = {"additionalProperties":false,"properties":{"order":{"default":"all","maxLength":8192,"title":"Order","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"InteractionViewSettings","type":"object"};
const schema42 = {"additionalProperties":false,"properties":{"counter":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Counter"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"service_status":{"default":"all","maxLength":8192,"title":"Service Status","type":"string"}},"title":"MomViewSettings","type":"object"};
const schema43 = {"additionalProperties":false,"properties":{"attribute":{"maxLength":8192,"minLength":1,"title":"Attribute","type":"string"},"object_key":{"maxLength":8192,"minLength":1,"title":"Object Key","type":"string"},"scope":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Scope"}},"required":["object_key","attribute"],"title":"ObjectChartChoice","type":"object"};
const schema44 = {"additionalProperties":false,"properties":{"scope":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Scope"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"default":"","maxLength":8192,"title":"Selected","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"ObjectViewSettings","type":"object"};
const schema45 = {"additionalProperties":false,"properties":{"owner_state":{"default":"all","maxLength":8192,"title":"Owner State","type":"string"},"request_status":{"default":"all","maxLength":8192,"title":"Request Status","type":"string"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"}},"title":"OwnershipViewSettings","type":"object"};
const schema46 = {"additionalProperties":false,"properties":{"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"phase":{"default":"all","maxLength":8192,"title":"Phase","type":"string"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"default":"all","maxLength":8192,"title":"Selected","type":"string"}},"title":"CoordinationViewSettings","type":"object"};
const schema47 = {"additionalProperties":false,"properties":{"evidence":{"default":"all","maxLength":8192,"title":"Evidence","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"RegionViewSettings","type":"object"};
const schema50 = {"additionalProperties":false,"properties":{"evidence":{"default":"all","maxLength":8192,"title":"Evidence","type":"string"},"focused":{"default":"all","maxLength":8192,"title":"Focused","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"source":{"default":"all","maxLength":8192,"title":"Source","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"TopologyViewSettings","type":"object"};

function validate21(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate21.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.alerts === undefined){
data.alerts = {"search":"","status":"all"};
}
if(data.declarations === undefined){
data.declarations = {"direction":"all","kind":"all","perspective":"all","search":"","status":"all"};
}
if(data.events === undefined){
data.events = {"family":"","federate":"","search":"","source":"","type":""};
}
if(data.federate_declarations === undefined){
data.federate_declarations = {"direction":"all","kind":"all","perspective":"all","search":"","status":"all"};
}
if(data.federate_events === undefined){
data.federate_events = {"family":"","federate":"","search":"","source":"","type":""};
}
if(data.federate_time === undefined){
data.federate_time = {"grants_only":false,"selected":null};
}
if(data.federation === undefined){
data.federation = {"distribution":"event_types"};
}
if(data.fom === undefined){
data.fom = {"kind":"all","model":"all","search":"","selected":null,"status":"all"};
}
if(data.interactions === undefined){
data.interactions = {"order":"all","search":"","status":"all"};
}
if(data.mom === undefined){
data.mom = {"counter":null,"scope":"all","search":"","service_status":"all"};
}
if(data.object_charts === undefined){
data.object_charts = [];
}
if(data.objects === undefined){
data.objects = {"scope":null,"search":"","selected":"","status":"all"};
}
if(data.ownership === undefined){
data.ownership = {"owner_state":"all","request_status":"all","scope":"all","search":""};
}
if(data.persistence === undefined){
data.persistence = {"kind":"all","phase":"all","scope":"all","search":"","selected":"all"};
}
if(data.regions === undefined){
data.regions = {"evidence":"all","kind":"all","search":"","selected":null,"status":"all"};
}
if(data.selected_federate === undefined){
data.selected_federate = "";
}
if(data.synchronization === undefined){
data.synchronization = {"kind":"all","phase":"all","scope":"all","search":"","selected":"all"};
}
if(data.time === undefined){
data.time = {"grants_only":false,"selected":null};
}
if(data.topology === undefined){
data.topology = {"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"};
}
if(data.view === undefined){
data.view = "federation";
}
if(data.window_seconds === undefined){
data.window_seconds = 300;
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(func0.call(schema32.properties, key0))){
validate21.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.alerts;
const _errs2 = errors;
const _errs3 = errors;
if(errors === _errs3){
if(data0 && typeof data0 == "object" && !Array.isArray(data0)){
if(data0.search === undefined){
data0.search = "";
}
if(data0.status === undefined){
data0.status = "all";
}
const _errs5 = errors;
for(const key1 of Object.keys(data0)){
if(!((key1 === "search") || (key1 === "status"))){
validate21.errors = [{instancePath:instancePath+"/alerts",schemaPath:"#/$defs/AlertViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key1},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs5 === errors){
let data1 = data0.search;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data1 === "string"){
if(func63(data1) > 2048){
validate21.errors = [{instancePath:instancePath+"/alerts/search",schemaPath:"#/$defs/AlertViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/alerts/search",schemaPath:"#/$defs/AlertViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid2 = _errs6 === errors;
if(valid2){
let data2 = data0.status;
const _errs8 = errors;
if(errors === _errs8){
if(typeof data2 === "string"){
if(func63(data2) > 8192){
validate21.errors = [{instancePath:instancePath+"/alerts/status",schemaPath:"#/$defs/AlertViewSettings/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/alerts/status",schemaPath:"#/$defs/AlertViewSettings/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid2 = _errs8 === errors;
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/alerts",schemaPath:"#/$defs/AlertViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data3 = data.declarations;
const _errs10 = errors;
const _errs11 = errors;
if(errors === _errs11){
if(data3 && typeof data3 == "object" && !Array.isArray(data3)){
if(data3.direction === undefined){
data3.direction = "all";
}
if(data3.kind === undefined){
data3.kind = "all";
}
if(data3.perspective === undefined){
data3.perspective = "all";
}
if(data3.search === undefined){
data3.search = "";
}
if(data3.status === undefined){
data3.status = "all";
}
const _errs13 = errors;
for(const key2 of Object.keys(data3)){
if(!(((((key2 === "direction") || (key2 === "kind")) || (key2 === "perspective")) || (key2 === "search")) || (key2 === "status"))){
validate21.errors = [{instancePath:instancePath+"/declarations",schemaPath:"#/$defs/DeclarationViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key2},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs13 === errors){
let data4 = data3.direction;
const _errs14 = errors;
if(errors === _errs14){
if(typeof data4 === "string"){
if(func63(data4) > 8192){
validate21.errors = [{instancePath:instancePath+"/declarations/direction",schemaPath:"#/$defs/DeclarationViewSettings/properties/direction/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/declarations/direction",schemaPath:"#/$defs/DeclarationViewSettings/properties/direction/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid4 = _errs14 === errors;
if(valid4){
let data5 = data3.kind;
const _errs16 = errors;
if(errors === _errs16){
if(typeof data5 === "string"){
if(func63(data5) > 8192){
validate21.errors = [{instancePath:instancePath+"/declarations/kind",schemaPath:"#/$defs/DeclarationViewSettings/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/declarations/kind",schemaPath:"#/$defs/DeclarationViewSettings/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid4 = _errs16 === errors;
if(valid4){
let data6 = data3.perspective;
const _errs18 = errors;
if(errors === _errs18){
if(typeof data6 === "string"){
if(func63(data6) > 8192){
validate21.errors = [{instancePath:instancePath+"/declarations/perspective",schemaPath:"#/$defs/DeclarationViewSettings/properties/perspective/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/declarations/perspective",schemaPath:"#/$defs/DeclarationViewSettings/properties/perspective/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid4 = _errs18 === errors;
if(valid4){
let data7 = data3.search;
const _errs20 = errors;
if(errors === _errs20){
if(typeof data7 === "string"){
if(func63(data7) > 2048){
validate21.errors = [{instancePath:instancePath+"/declarations/search",schemaPath:"#/$defs/DeclarationViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/declarations/search",schemaPath:"#/$defs/DeclarationViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid4 = _errs20 === errors;
if(valid4){
let data8 = data3.status;
const _errs22 = errors;
if(errors === _errs22){
if(typeof data8 === "string"){
if(func63(data8) > 8192){
validate21.errors = [{instancePath:instancePath+"/declarations/status",schemaPath:"#/$defs/DeclarationViewSettings/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/declarations/status",schemaPath:"#/$defs/DeclarationViewSettings/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid4 = _errs22 === errors;
}
}
}
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/declarations",schemaPath:"#/$defs/DeclarationViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs10 === errors;
if(valid0){
let data9 = data.events;
const _errs24 = errors;
const _errs25 = errors;
if(errors === _errs25){
if(data9 && typeof data9 == "object" && !Array.isArray(data9)){
if(data9.family === undefined){
data9.family = "";
}
if(data9.federate === undefined){
data9.federate = "";
}
if(data9.search === undefined){
data9.search = "";
}
if(data9.source === undefined){
data9.source = "";
}
if(data9.type === undefined){
data9.type = "";
}
const _errs27 = errors;
for(const key3 of Object.keys(data9)){
if(!(((((key3 === "family") || (key3 === "federate")) || (key3 === "search")) || (key3 === "source")) || (key3 === "type"))){
validate21.errors = [{instancePath:instancePath+"/events",schemaPath:"#/$defs/EventViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key3},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs27 === errors){
let data10 = data9.family;
const _errs28 = errors;
if(errors === _errs28){
if(typeof data10 === "string"){
if(func63(data10) > 8192){
validate21.errors = [{instancePath:instancePath+"/events/family",schemaPath:"#/$defs/EventViewSettings/properties/family/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/events/family",schemaPath:"#/$defs/EventViewSettings/properties/family/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid6 = _errs28 === errors;
if(valid6){
let data11 = data9.federate;
const _errs30 = errors;
if(errors === _errs30){
if(typeof data11 === "string"){
if(func63(data11) > 8192){
validate21.errors = [{instancePath:instancePath+"/events/federate",schemaPath:"#/$defs/EventViewSettings/properties/federate/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/events/federate",schemaPath:"#/$defs/EventViewSettings/properties/federate/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid6 = _errs30 === errors;
if(valid6){
let data12 = data9.search;
const _errs32 = errors;
if(errors === _errs32){
if(typeof data12 === "string"){
if(func63(data12) > 2048){
validate21.errors = [{instancePath:instancePath+"/events/search",schemaPath:"#/$defs/EventViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/events/search",schemaPath:"#/$defs/EventViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid6 = _errs32 === errors;
if(valid6){
let data13 = data9.source;
const _errs34 = errors;
if(errors === _errs34){
if(typeof data13 === "string"){
if(func63(data13) > 8192){
validate21.errors = [{instancePath:instancePath+"/events/source",schemaPath:"#/$defs/EventViewSettings/properties/source/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/events/source",schemaPath:"#/$defs/EventViewSettings/properties/source/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid6 = _errs34 === errors;
if(valid6){
let data14 = data9.type;
const _errs36 = errors;
if(errors === _errs36){
if(typeof data14 === "string"){
if(func63(data14) > 8192){
validate21.errors = [{instancePath:instancePath+"/events/type",schemaPath:"#/$defs/EventViewSettings/properties/type/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/events/type",schemaPath:"#/$defs/EventViewSettings/properties/type/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid6 = _errs36 === errors;
}
}
}
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/events",schemaPath:"#/$defs/EventViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs24 === errors;
if(valid0){
let data15 = data.federate_declarations;
const _errs38 = errors;
const _errs39 = errors;
if(errors === _errs39){
if(data15 && typeof data15 == "object" && !Array.isArray(data15)){
if(data15.direction === undefined){
data15.direction = "all";
}
if(data15.kind === undefined){
data15.kind = "all";
}
if(data15.perspective === undefined){
data15.perspective = "all";
}
if(data15.search === undefined){
data15.search = "";
}
if(data15.status === undefined){
data15.status = "all";
}
const _errs41 = errors;
for(const key4 of Object.keys(data15)){
if(!(((((key4 === "direction") || (key4 === "kind")) || (key4 === "perspective")) || (key4 === "search")) || (key4 === "status"))){
validate21.errors = [{instancePath:instancePath+"/federate_declarations",schemaPath:"#/$defs/DeclarationViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key4},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs41 === errors){
let data16 = data15.direction;
const _errs42 = errors;
if(errors === _errs42){
if(typeof data16 === "string"){
if(func63(data16) > 8192){
validate21.errors = [{instancePath:instancePath+"/federate_declarations/direction",schemaPath:"#/$defs/DeclarationViewSettings/properties/direction/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/federate_declarations/direction",schemaPath:"#/$defs/DeclarationViewSettings/properties/direction/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid8 = _errs42 === errors;
if(valid8){
let data17 = data15.kind;
const _errs44 = errors;
if(errors === _errs44){
if(typeof data17 === "string"){
if(func63(data17) > 8192){
validate21.errors = [{instancePath:instancePath+"/federate_declarations/kind",schemaPath:"#/$defs/DeclarationViewSettings/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/federate_declarations/kind",schemaPath:"#/$defs/DeclarationViewSettings/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid8 = _errs44 === errors;
if(valid8){
let data18 = data15.perspective;
const _errs46 = errors;
if(errors === _errs46){
if(typeof data18 === "string"){
if(func63(data18) > 8192){
validate21.errors = [{instancePath:instancePath+"/federate_declarations/perspective",schemaPath:"#/$defs/DeclarationViewSettings/properties/perspective/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/federate_declarations/perspective",schemaPath:"#/$defs/DeclarationViewSettings/properties/perspective/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid8 = _errs46 === errors;
if(valid8){
let data19 = data15.search;
const _errs48 = errors;
if(errors === _errs48){
if(typeof data19 === "string"){
if(func63(data19) > 2048){
validate21.errors = [{instancePath:instancePath+"/federate_declarations/search",schemaPath:"#/$defs/DeclarationViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/federate_declarations/search",schemaPath:"#/$defs/DeclarationViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid8 = _errs48 === errors;
if(valid8){
let data20 = data15.status;
const _errs50 = errors;
if(errors === _errs50){
if(typeof data20 === "string"){
if(func63(data20) > 8192){
validate21.errors = [{instancePath:instancePath+"/federate_declarations/status",schemaPath:"#/$defs/DeclarationViewSettings/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/federate_declarations/status",schemaPath:"#/$defs/DeclarationViewSettings/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid8 = _errs50 === errors;
}
}
}
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/federate_declarations",schemaPath:"#/$defs/DeclarationViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs38 === errors;
if(valid0){
let data21 = data.federate_events;
const _errs52 = errors;
const _errs53 = errors;
if(errors === _errs53){
if(data21 && typeof data21 == "object" && !Array.isArray(data21)){
if(data21.family === undefined){
data21.family = "";
}
if(data21.federate === undefined){
data21.federate = "";
}
if(data21.search === undefined){
data21.search = "";
}
if(data21.source === undefined){
data21.source = "";
}
if(data21.type === undefined){
data21.type = "";
}
const _errs55 = errors;
for(const key5 of Object.keys(data21)){
if(!(((((key5 === "family") || (key5 === "federate")) || (key5 === "search")) || (key5 === "source")) || (key5 === "type"))){
validate21.errors = [{instancePath:instancePath+"/federate_events",schemaPath:"#/$defs/EventViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key5},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs55 === errors){
let data22 = data21.family;
const _errs56 = errors;
if(errors === _errs56){
if(typeof data22 === "string"){
if(func63(data22) > 8192){
validate21.errors = [{instancePath:instancePath+"/federate_events/family",schemaPath:"#/$defs/EventViewSettings/properties/family/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/federate_events/family",schemaPath:"#/$defs/EventViewSettings/properties/family/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid10 = _errs56 === errors;
if(valid10){
let data23 = data21.federate;
const _errs58 = errors;
if(errors === _errs58){
if(typeof data23 === "string"){
if(func63(data23) > 8192){
validate21.errors = [{instancePath:instancePath+"/federate_events/federate",schemaPath:"#/$defs/EventViewSettings/properties/federate/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/federate_events/federate",schemaPath:"#/$defs/EventViewSettings/properties/federate/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid10 = _errs58 === errors;
if(valid10){
let data24 = data21.search;
const _errs60 = errors;
if(errors === _errs60){
if(typeof data24 === "string"){
if(func63(data24) > 2048){
validate21.errors = [{instancePath:instancePath+"/federate_events/search",schemaPath:"#/$defs/EventViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/federate_events/search",schemaPath:"#/$defs/EventViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid10 = _errs60 === errors;
if(valid10){
let data25 = data21.source;
const _errs62 = errors;
if(errors === _errs62){
if(typeof data25 === "string"){
if(func63(data25) > 8192){
validate21.errors = [{instancePath:instancePath+"/federate_events/source",schemaPath:"#/$defs/EventViewSettings/properties/source/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/federate_events/source",schemaPath:"#/$defs/EventViewSettings/properties/source/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid10 = _errs62 === errors;
if(valid10){
let data26 = data21.type;
const _errs64 = errors;
if(errors === _errs64){
if(typeof data26 === "string"){
if(func63(data26) > 8192){
validate21.errors = [{instancePath:instancePath+"/federate_events/type",schemaPath:"#/$defs/EventViewSettings/properties/type/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/federate_events/type",schemaPath:"#/$defs/EventViewSettings/properties/type/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid10 = _errs64 === errors;
}
}
}
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/federate_events",schemaPath:"#/$defs/EventViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs52 === errors;
if(valid0){
let data27 = data.federate_time;
const _errs66 = errors;
const _errs67 = errors;
if(errors === _errs67){
if(data27 && typeof data27 == "object" && !Array.isArray(data27)){
if(data27.grants_only === undefined){
data27.grants_only = false;
}
if(data27.selected === undefined){
data27.selected = null;
}
const _errs69 = errors;
for(const key6 of Object.keys(data27)){
if(!((key6 === "grants_only") || (key6 === "selected"))){
validate21.errors = [{instancePath:instancePath+"/federate_time",schemaPath:"#/$defs/TimeViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key6},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs69 === errors){
const _errs70 = errors;
if(typeof data27.grants_only !== "boolean"){
validate21.errors = [{instancePath:instancePath+"/federate_time/grants_only",schemaPath:"#/$defs/TimeViewSettings/properties/grants_only/type",keyword:"type",params:{type: "boolean"},message:"must be boolean"}];
return false;
}
var valid12 = _errs70 === errors;
if(valid12){
let data29 = data27.selected;
const _errs72 = errors;
const _errs73 = errors;
let valid13 = false;
const _errs74 = errors;
if(errors === _errs74){
if(typeof data29 === "string"){
if(func63(data29) > 8192){
const err0 = {instancePath:instancePath+"/federate_time/selected",schemaPath:"#/$defs/TimeViewSettings/properties/selected/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
if(vErrors === null){
vErrors = [err0];
}
else {
vErrors.push(err0);
}
errors++;
}
}
else {
const err1 = {instancePath:instancePath+"/federate_time/selected",schemaPath:"#/$defs/TimeViewSettings/properties/selected/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err1];
}
else {
vErrors.push(err1);
}
errors++;
}
}
var _valid0 = _errs74 === errors;
valid13 = valid13 || _valid0;
const _errs76 = errors;
if(data29 !== null){
const err2 = {instancePath:instancePath+"/federate_time/selected",schemaPath:"#/$defs/TimeViewSettings/properties/selected/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err2];
}
else {
vErrors.push(err2);
}
errors++;
}
var _valid0 = _errs76 === errors;
valid13 = valid13 || _valid0;
if(!valid13){
const err3 = {instancePath:instancePath+"/federate_time/selected",schemaPath:"#/$defs/TimeViewSettings/properties/selected/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err3];
}
else {
vErrors.push(err3);
}
errors++;
validate21.errors = vErrors;
return false;
}
else {
errors = _errs73;
if(vErrors !== null){
if(_errs73){
vErrors.length = _errs73;
}
else {
vErrors = null;
}
}
}
var valid12 = _errs72 === errors;
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/federate_time",schemaPath:"#/$defs/TimeViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs66 === errors;
if(valid0){
let data30 = data.federation;
const _errs78 = errors;
const _errs79 = errors;
if(errors === _errs79){
if(data30 && typeof data30 == "object" && !Array.isArray(data30)){
if(data30.distribution === undefined){
data30.distribution = "event_types";
}
const _errs81 = errors;
for(const key7 of Object.keys(data30)){
if(!(key7 === "distribution")){
validate21.errors = [{instancePath:instancePath+"/federation",schemaPath:"#/$defs/FederationViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key7},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs81 === errors){
let data31 = data30.distribution;
if(typeof data31 !== "string"){
validate21.errors = [{instancePath:instancePath+"/federation/distribution",schemaPath:"#/$defs/FederationViewSettings/properties/distribution/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
if(!(((data31 === "event_types") || (data31 === "sources")) || (data31 === "families"))){
validate21.errors = [{instancePath:instancePath+"/federation/distribution",schemaPath:"#/$defs/FederationViewSettings/properties/distribution/enum",keyword:"enum",params:{allowedValues: schema39.properties.distribution.enum},message:"must be equal to one of the allowed values"}];
return false;
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/federation",schemaPath:"#/$defs/FederationViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs78 === errors;
if(valid0){
let data32 = data.fom;
const _errs84 = errors;
const _errs85 = errors;
if(errors === _errs85){
if(data32 && typeof data32 == "object" && !Array.isArray(data32)){
if(data32.kind === undefined){
data32.kind = "all";
}
if(data32.model === undefined){
data32.model = "all";
}
if(data32.search === undefined){
data32.search = "";
}
if(data32.selected === undefined){
data32.selected = null;
}
if(data32.status === undefined){
data32.status = "all";
}
const _errs87 = errors;
for(const key8 of Object.keys(data32)){
if(!(((((key8 === "kind") || (key8 === "model")) || (key8 === "search")) || (key8 === "selected")) || (key8 === "status"))){
validate21.errors = [{instancePath:instancePath+"/fom",schemaPath:"#/$defs/FomViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key8},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs87 === errors){
let data33 = data32.kind;
const _errs88 = errors;
if(errors === _errs88){
if(typeof data33 === "string"){
if(func63(data33) > 8192){
validate21.errors = [{instancePath:instancePath+"/fom/kind",schemaPath:"#/$defs/FomViewSettings/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/fom/kind",schemaPath:"#/$defs/FomViewSettings/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid17 = _errs88 === errors;
if(valid17){
let data34 = data32.model;
const _errs90 = errors;
if(errors === _errs90){
if(typeof data34 === "string"){
if(func63(data34) > 8192){
validate21.errors = [{instancePath:instancePath+"/fom/model",schemaPath:"#/$defs/FomViewSettings/properties/model/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/fom/model",schemaPath:"#/$defs/FomViewSettings/properties/model/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid17 = _errs90 === errors;
if(valid17){
let data35 = data32.search;
const _errs92 = errors;
if(errors === _errs92){
if(typeof data35 === "string"){
if(func63(data35) > 2048){
validate21.errors = [{instancePath:instancePath+"/fom/search",schemaPath:"#/$defs/FomViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/fom/search",schemaPath:"#/$defs/FomViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid17 = _errs92 === errors;
if(valid17){
let data36 = data32.selected;
const _errs94 = errors;
const _errs95 = errors;
let valid18 = false;
const _errs96 = errors;
if(errors === _errs96){
if(typeof data36 === "string"){
if(func63(data36) > 8192){
const err4 = {instancePath:instancePath+"/fom/selected",schemaPath:"#/$defs/FomViewSettings/properties/selected/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
if(vErrors === null){
vErrors = [err4];
}
else {
vErrors.push(err4);
}
errors++;
}
}
else {
const err5 = {instancePath:instancePath+"/fom/selected",schemaPath:"#/$defs/FomViewSettings/properties/selected/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err5];
}
else {
vErrors.push(err5);
}
errors++;
}
}
var _valid1 = _errs96 === errors;
valid18 = valid18 || _valid1;
const _errs98 = errors;
if(data36 !== null){
const err6 = {instancePath:instancePath+"/fom/selected",schemaPath:"#/$defs/FomViewSettings/properties/selected/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err6];
}
else {
vErrors.push(err6);
}
errors++;
}
var _valid1 = _errs98 === errors;
valid18 = valid18 || _valid1;
if(!valid18){
const err7 = {instancePath:instancePath+"/fom/selected",schemaPath:"#/$defs/FomViewSettings/properties/selected/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err7];
}
else {
vErrors.push(err7);
}
errors++;
validate21.errors = vErrors;
return false;
}
else {
errors = _errs95;
if(vErrors !== null){
if(_errs95){
vErrors.length = _errs95;
}
else {
vErrors = null;
}
}
}
var valid17 = _errs94 === errors;
if(valid17){
let data37 = data32.status;
const _errs100 = errors;
if(errors === _errs100){
if(typeof data37 === "string"){
if(func63(data37) > 8192){
validate21.errors = [{instancePath:instancePath+"/fom/status",schemaPath:"#/$defs/FomViewSettings/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/fom/status",schemaPath:"#/$defs/FomViewSettings/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid17 = _errs100 === errors;
}
}
}
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/fom",schemaPath:"#/$defs/FomViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs84 === errors;
if(valid0){
let data38 = data.interactions;
const _errs102 = errors;
const _errs103 = errors;
if(errors === _errs103){
if(data38 && typeof data38 == "object" && !Array.isArray(data38)){
if(data38.order === undefined){
data38.order = "all";
}
if(data38.search === undefined){
data38.search = "";
}
if(data38.status === undefined){
data38.status = "all";
}
const _errs105 = errors;
for(const key9 of Object.keys(data38)){
if(!(((key9 === "order") || (key9 === "search")) || (key9 === "status"))){
validate21.errors = [{instancePath:instancePath+"/interactions",schemaPath:"#/$defs/InteractionViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key9},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs105 === errors){
let data39 = data38.order;
const _errs106 = errors;
if(errors === _errs106){
if(typeof data39 === "string"){
if(func63(data39) > 8192){
validate21.errors = [{instancePath:instancePath+"/interactions/order",schemaPath:"#/$defs/InteractionViewSettings/properties/order/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/interactions/order",schemaPath:"#/$defs/InteractionViewSettings/properties/order/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid20 = _errs106 === errors;
if(valid20){
let data40 = data38.search;
const _errs108 = errors;
if(errors === _errs108){
if(typeof data40 === "string"){
if(func63(data40) > 2048){
validate21.errors = [{instancePath:instancePath+"/interactions/search",schemaPath:"#/$defs/InteractionViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/interactions/search",schemaPath:"#/$defs/InteractionViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid20 = _errs108 === errors;
if(valid20){
let data41 = data38.status;
const _errs110 = errors;
if(errors === _errs110){
if(typeof data41 === "string"){
if(func63(data41) > 8192){
validate21.errors = [{instancePath:instancePath+"/interactions/status",schemaPath:"#/$defs/InteractionViewSettings/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/interactions/status",schemaPath:"#/$defs/InteractionViewSettings/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid20 = _errs110 === errors;
}
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/interactions",schemaPath:"#/$defs/InteractionViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs102 === errors;
if(valid0){
let data42 = data.mom;
const _errs112 = errors;
const _errs113 = errors;
if(errors === _errs113){
if(data42 && typeof data42 == "object" && !Array.isArray(data42)){
if(data42.counter === undefined){
data42.counter = null;
}
if(data42.scope === undefined){
data42.scope = "all";
}
if(data42.search === undefined){
data42.search = "";
}
if(data42.service_status === undefined){
data42.service_status = "all";
}
const _errs115 = errors;
for(const key10 of Object.keys(data42)){
if(!((((key10 === "counter") || (key10 === "scope")) || (key10 === "search")) || (key10 === "service_status"))){
validate21.errors = [{instancePath:instancePath+"/mom",schemaPath:"#/$defs/MomViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key10},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs115 === errors){
let data43 = data42.counter;
const _errs116 = errors;
const _errs117 = errors;
let valid23 = false;
const _errs118 = errors;
if(errors === _errs118){
if(typeof data43 === "string"){
if(func63(data43) > 8192){
const err8 = {instancePath:instancePath+"/mom/counter",schemaPath:"#/$defs/MomViewSettings/properties/counter/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
if(vErrors === null){
vErrors = [err8];
}
else {
vErrors.push(err8);
}
errors++;
}
}
else {
const err9 = {instancePath:instancePath+"/mom/counter",schemaPath:"#/$defs/MomViewSettings/properties/counter/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err9];
}
else {
vErrors.push(err9);
}
errors++;
}
}
var _valid2 = _errs118 === errors;
valid23 = valid23 || _valid2;
const _errs120 = errors;
if(data43 !== null){
const err10 = {instancePath:instancePath+"/mom/counter",schemaPath:"#/$defs/MomViewSettings/properties/counter/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err10];
}
else {
vErrors.push(err10);
}
errors++;
}
var _valid2 = _errs120 === errors;
valid23 = valid23 || _valid2;
if(!valid23){
const err11 = {instancePath:instancePath+"/mom/counter",schemaPath:"#/$defs/MomViewSettings/properties/counter/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err11];
}
else {
vErrors.push(err11);
}
errors++;
validate21.errors = vErrors;
return false;
}
else {
errors = _errs117;
if(vErrors !== null){
if(_errs117){
vErrors.length = _errs117;
}
else {
vErrors = null;
}
}
}
var valid22 = _errs116 === errors;
if(valid22){
let data44 = data42.scope;
const _errs122 = errors;
if(errors === _errs122){
if(typeof data44 === "string"){
if(func63(data44) > 8192){
validate21.errors = [{instancePath:instancePath+"/mom/scope",schemaPath:"#/$defs/MomViewSettings/properties/scope/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/mom/scope",schemaPath:"#/$defs/MomViewSettings/properties/scope/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid22 = _errs122 === errors;
if(valid22){
let data45 = data42.search;
const _errs124 = errors;
if(errors === _errs124){
if(typeof data45 === "string"){
if(func63(data45) > 2048){
validate21.errors = [{instancePath:instancePath+"/mom/search",schemaPath:"#/$defs/MomViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/mom/search",schemaPath:"#/$defs/MomViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid22 = _errs124 === errors;
if(valid22){
let data46 = data42.service_status;
const _errs126 = errors;
if(errors === _errs126){
if(typeof data46 === "string"){
if(func63(data46) > 8192){
validate21.errors = [{instancePath:instancePath+"/mom/service_status",schemaPath:"#/$defs/MomViewSettings/properties/service_status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/mom/service_status",schemaPath:"#/$defs/MomViewSettings/properties/service_status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid22 = _errs126 === errors;
}
}
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/mom",schemaPath:"#/$defs/MomViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs112 === errors;
if(valid0){
let data47 = data.object_charts;
const _errs128 = errors;
if(errors === _errs128){
if(Array.isArray(data47)){
if(data47.length > 64){
validate21.errors = [{instancePath:instancePath+"/object_charts",schemaPath:"#/properties/object_charts/maxItems",keyword:"maxItems",params:{limit: 64},message:"must NOT have more than 64 items"}];
return false;
}
else {
var valid24 = true;
const len0 = data47.length;
for(let i0=0; i0<len0; i0++){
let data48 = data47[i0];
const _errs130 = errors;
const _errs131 = errors;
if(errors === _errs131){
if(data48 && typeof data48 == "object" && !Array.isArray(data48)){
if(data48.scope === undefined){
data48.scope = null;
}
let missing0;
if((((data48.object_key === undefined) || (!(func0.call(data48, "object_key")))) && (missing0 = "object_key")) || (((data48.attribute === undefined) || (!(func0.call(data48, "attribute")))) && (missing0 = "attribute"))){
validate21.errors = [{instancePath:instancePath+"/object_charts/" + i0,schemaPath:"#/$defs/ObjectChartChoice/required",keyword:"required",params:{missingProperty: missing0},message:"must have required property '"+missing0+"'"}];
return false;
}
else {
const _errs133 = errors;
for(const key11 of Object.keys(data48)){
if(!(((key11 === "attribute") || (key11 === "object_key")) || (key11 === "scope"))){
validate21.errors = [{instancePath:instancePath+"/object_charts/" + i0,schemaPath:"#/$defs/ObjectChartChoice/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key11},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs133 === errors){
if(data48.attribute !== undefined && func0.call(data48, "attribute")){
let data49 = data48.attribute;
const _errs134 = errors;
if(errors === _errs134){
if(typeof data49 === "string"){
if(func63(data49) > 8192){
validate21.errors = [{instancePath:instancePath+"/object_charts/" + i0+"/attribute",schemaPath:"#/$defs/ObjectChartChoice/properties/attribute/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
else {
if(func63(data49) < 1){
validate21.errors = [{instancePath:instancePath+"/object_charts/" + i0+"/attribute",schemaPath:"#/$defs/ObjectChartChoice/properties/attribute/minLength",keyword:"minLength",params:{limit: 1},message:"must NOT have fewer than 1 characters"}];
return false;
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/object_charts/" + i0+"/attribute",schemaPath:"#/$defs/ObjectChartChoice/properties/attribute/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid26 = _errs134 === errors;
}
else {
var valid26 = true;
}
if(valid26){
if(data48.object_key !== undefined && func0.call(data48, "object_key")){
let data50 = data48.object_key;
const _errs136 = errors;
if(errors === _errs136){
if(typeof data50 === "string"){
if(func63(data50) > 8192){
validate21.errors = [{instancePath:instancePath+"/object_charts/" + i0+"/object_key",schemaPath:"#/$defs/ObjectChartChoice/properties/object_key/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
else {
if(func63(data50) < 1){
validate21.errors = [{instancePath:instancePath+"/object_charts/" + i0+"/object_key",schemaPath:"#/$defs/ObjectChartChoice/properties/object_key/minLength",keyword:"minLength",params:{limit: 1},message:"must NOT have fewer than 1 characters"}];
return false;
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/object_charts/" + i0+"/object_key",schemaPath:"#/$defs/ObjectChartChoice/properties/object_key/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid26 = _errs136 === errors;
}
else {
var valid26 = true;
}
if(valid26){
let data51 = data48.scope;
const _errs138 = errors;
const _errs139 = errors;
let valid27 = false;
const _errs140 = errors;
if(errors === _errs140){
if(typeof data51 === "string"){
if(func63(data51) > 8192){
const err12 = {instancePath:instancePath+"/object_charts/" + i0+"/scope",schemaPath:"#/$defs/ObjectChartChoice/properties/scope/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
if(vErrors === null){
vErrors = [err12];
}
else {
vErrors.push(err12);
}
errors++;
}
}
else {
const err13 = {instancePath:instancePath+"/object_charts/" + i0+"/scope",schemaPath:"#/$defs/ObjectChartChoice/properties/scope/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err13];
}
else {
vErrors.push(err13);
}
errors++;
}
}
var _valid3 = _errs140 === errors;
valid27 = valid27 || _valid3;
const _errs142 = errors;
if(data51 !== null){
const err14 = {instancePath:instancePath+"/object_charts/" + i0+"/scope",schemaPath:"#/$defs/ObjectChartChoice/properties/scope/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err14];
}
else {
vErrors.push(err14);
}
errors++;
}
var _valid3 = _errs142 === errors;
valid27 = valid27 || _valid3;
if(!valid27){
const err15 = {instancePath:instancePath+"/object_charts/" + i0+"/scope",schemaPath:"#/$defs/ObjectChartChoice/properties/scope/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err15];
}
else {
vErrors.push(err15);
}
errors++;
validate21.errors = vErrors;
return false;
}
else {
errors = _errs139;
if(vErrors !== null){
if(_errs139){
vErrors.length = _errs139;
}
else {
vErrors = null;
}
}
}
var valid26 = _errs138 === errors;
}
}
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/object_charts/" + i0,schemaPath:"#/$defs/ObjectChartChoice/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid24 = _errs130 === errors;
if(!valid24){
break;
}
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/object_charts",schemaPath:"#/properties/object_charts/type",keyword:"type",params:{type: "array"},message:"must be array"}];
return false;
}
}
var valid0 = _errs128 === errors;
if(valid0){
let data52 = data.objects;
const _errs144 = errors;
const _errs145 = errors;
if(errors === _errs145){
if(data52 && typeof data52 == "object" && !Array.isArray(data52)){
if(data52.scope === undefined){
data52.scope = null;
}
if(data52.search === undefined){
data52.search = "";
}
if(data52.selected === undefined){
data52.selected = "";
}
if(data52.status === undefined){
data52.status = "all";
}
const _errs147 = errors;
for(const key12 of Object.keys(data52)){
if(!((((key12 === "scope") || (key12 === "search")) || (key12 === "selected")) || (key12 === "status"))){
validate21.errors = [{instancePath:instancePath+"/objects",schemaPath:"#/$defs/ObjectViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key12},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs147 === errors){
let data53 = data52.scope;
const _errs148 = errors;
const _errs149 = errors;
let valid30 = false;
const _errs150 = errors;
if(errors === _errs150){
if(typeof data53 === "string"){
if(func63(data53) > 8192){
const err16 = {instancePath:instancePath+"/objects/scope",schemaPath:"#/$defs/ObjectViewSettings/properties/scope/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
if(vErrors === null){
vErrors = [err16];
}
else {
vErrors.push(err16);
}
errors++;
}
}
else {
const err17 = {instancePath:instancePath+"/objects/scope",schemaPath:"#/$defs/ObjectViewSettings/properties/scope/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err17];
}
else {
vErrors.push(err17);
}
errors++;
}
}
var _valid4 = _errs150 === errors;
valid30 = valid30 || _valid4;
const _errs152 = errors;
if(data53 !== null){
const err18 = {instancePath:instancePath+"/objects/scope",schemaPath:"#/$defs/ObjectViewSettings/properties/scope/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err18];
}
else {
vErrors.push(err18);
}
errors++;
}
var _valid4 = _errs152 === errors;
valid30 = valid30 || _valid4;
if(!valid30){
const err19 = {instancePath:instancePath+"/objects/scope",schemaPath:"#/$defs/ObjectViewSettings/properties/scope/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err19];
}
else {
vErrors.push(err19);
}
errors++;
validate21.errors = vErrors;
return false;
}
else {
errors = _errs149;
if(vErrors !== null){
if(_errs149){
vErrors.length = _errs149;
}
else {
vErrors = null;
}
}
}
var valid29 = _errs148 === errors;
if(valid29){
let data54 = data52.search;
const _errs154 = errors;
if(errors === _errs154){
if(typeof data54 === "string"){
if(func63(data54) > 2048){
validate21.errors = [{instancePath:instancePath+"/objects/search",schemaPath:"#/$defs/ObjectViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/objects/search",schemaPath:"#/$defs/ObjectViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid29 = _errs154 === errors;
if(valid29){
let data55 = data52.selected;
const _errs156 = errors;
if(errors === _errs156){
if(typeof data55 === "string"){
if(func63(data55) > 8192){
validate21.errors = [{instancePath:instancePath+"/objects/selected",schemaPath:"#/$defs/ObjectViewSettings/properties/selected/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/objects/selected",schemaPath:"#/$defs/ObjectViewSettings/properties/selected/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid29 = _errs156 === errors;
if(valid29){
let data56 = data52.status;
const _errs158 = errors;
if(errors === _errs158){
if(typeof data56 === "string"){
if(func63(data56) > 8192){
validate21.errors = [{instancePath:instancePath+"/objects/status",schemaPath:"#/$defs/ObjectViewSettings/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/objects/status",schemaPath:"#/$defs/ObjectViewSettings/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid29 = _errs158 === errors;
}
}
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/objects",schemaPath:"#/$defs/ObjectViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs144 === errors;
if(valid0){
let data57 = data.ownership;
const _errs160 = errors;
const _errs161 = errors;
if(errors === _errs161){
if(data57 && typeof data57 == "object" && !Array.isArray(data57)){
if(data57.owner_state === undefined){
data57.owner_state = "all";
}
if(data57.request_status === undefined){
data57.request_status = "all";
}
if(data57.scope === undefined){
data57.scope = "all";
}
if(data57.search === undefined){
data57.search = "";
}
const _errs163 = errors;
for(const key13 of Object.keys(data57)){
if(!((((key13 === "owner_state") || (key13 === "request_status")) || (key13 === "scope")) || (key13 === "search"))){
validate21.errors = [{instancePath:instancePath+"/ownership",schemaPath:"#/$defs/OwnershipViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key13},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs163 === errors){
let data58 = data57.owner_state;
const _errs164 = errors;
if(errors === _errs164){
if(typeof data58 === "string"){
if(func63(data58) > 8192){
validate21.errors = [{instancePath:instancePath+"/ownership/owner_state",schemaPath:"#/$defs/OwnershipViewSettings/properties/owner_state/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/ownership/owner_state",schemaPath:"#/$defs/OwnershipViewSettings/properties/owner_state/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid32 = _errs164 === errors;
if(valid32){
let data59 = data57.request_status;
const _errs166 = errors;
if(errors === _errs166){
if(typeof data59 === "string"){
if(func63(data59) > 8192){
validate21.errors = [{instancePath:instancePath+"/ownership/request_status",schemaPath:"#/$defs/OwnershipViewSettings/properties/request_status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/ownership/request_status",schemaPath:"#/$defs/OwnershipViewSettings/properties/request_status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid32 = _errs166 === errors;
if(valid32){
let data60 = data57.scope;
const _errs168 = errors;
if(errors === _errs168){
if(typeof data60 === "string"){
if(func63(data60) > 8192){
validate21.errors = [{instancePath:instancePath+"/ownership/scope",schemaPath:"#/$defs/OwnershipViewSettings/properties/scope/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/ownership/scope",schemaPath:"#/$defs/OwnershipViewSettings/properties/scope/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid32 = _errs168 === errors;
if(valid32){
let data61 = data57.search;
const _errs170 = errors;
if(errors === _errs170){
if(typeof data61 === "string"){
if(func63(data61) > 2048){
validate21.errors = [{instancePath:instancePath+"/ownership/search",schemaPath:"#/$defs/OwnershipViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/ownership/search",schemaPath:"#/$defs/OwnershipViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid32 = _errs170 === errors;
}
}
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/ownership",schemaPath:"#/$defs/OwnershipViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs160 === errors;
if(valid0){
let data62 = data.persistence;
const _errs172 = errors;
const _errs173 = errors;
if(errors === _errs173){
if(data62 && typeof data62 == "object" && !Array.isArray(data62)){
if(data62.kind === undefined){
data62.kind = "all";
}
if(data62.phase === undefined){
data62.phase = "all";
}
if(data62.scope === undefined){
data62.scope = "all";
}
if(data62.search === undefined){
data62.search = "";
}
if(data62.selected === undefined){
data62.selected = "all";
}
const _errs175 = errors;
for(const key14 of Object.keys(data62)){
if(!(((((key14 === "kind") || (key14 === "phase")) || (key14 === "scope")) || (key14 === "search")) || (key14 === "selected"))){
validate21.errors = [{instancePath:instancePath+"/persistence",schemaPath:"#/$defs/CoordinationViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key14},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs175 === errors){
let data63 = data62.kind;
const _errs176 = errors;
if(errors === _errs176){
if(typeof data63 === "string"){
if(func63(data63) > 8192){
validate21.errors = [{instancePath:instancePath+"/persistence/kind",schemaPath:"#/$defs/CoordinationViewSettings/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/persistence/kind",schemaPath:"#/$defs/CoordinationViewSettings/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid34 = _errs176 === errors;
if(valid34){
let data64 = data62.phase;
const _errs178 = errors;
if(errors === _errs178){
if(typeof data64 === "string"){
if(func63(data64) > 8192){
validate21.errors = [{instancePath:instancePath+"/persistence/phase",schemaPath:"#/$defs/CoordinationViewSettings/properties/phase/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/persistence/phase",schemaPath:"#/$defs/CoordinationViewSettings/properties/phase/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid34 = _errs178 === errors;
if(valid34){
let data65 = data62.scope;
const _errs180 = errors;
if(errors === _errs180){
if(typeof data65 === "string"){
if(func63(data65) > 8192){
validate21.errors = [{instancePath:instancePath+"/persistence/scope",schemaPath:"#/$defs/CoordinationViewSettings/properties/scope/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/persistence/scope",schemaPath:"#/$defs/CoordinationViewSettings/properties/scope/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid34 = _errs180 === errors;
if(valid34){
let data66 = data62.search;
const _errs182 = errors;
if(errors === _errs182){
if(typeof data66 === "string"){
if(func63(data66) > 2048){
validate21.errors = [{instancePath:instancePath+"/persistence/search",schemaPath:"#/$defs/CoordinationViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/persistence/search",schemaPath:"#/$defs/CoordinationViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid34 = _errs182 === errors;
if(valid34){
let data67 = data62.selected;
const _errs184 = errors;
if(errors === _errs184){
if(typeof data67 === "string"){
if(func63(data67) > 8192){
validate21.errors = [{instancePath:instancePath+"/persistence/selected",schemaPath:"#/$defs/CoordinationViewSettings/properties/selected/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/persistence/selected",schemaPath:"#/$defs/CoordinationViewSettings/properties/selected/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid34 = _errs184 === errors;
}
}
}
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/persistence",schemaPath:"#/$defs/CoordinationViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs172 === errors;
if(valid0){
let data68 = data.regions;
const _errs186 = errors;
const _errs187 = errors;
if(errors === _errs187){
if(data68 && typeof data68 == "object" && !Array.isArray(data68)){
if(data68.evidence === undefined){
data68.evidence = "all";
}
if(data68.kind === undefined){
data68.kind = "all";
}
if(data68.search === undefined){
data68.search = "";
}
if(data68.selected === undefined){
data68.selected = null;
}
if(data68.status === undefined){
data68.status = "all";
}
const _errs189 = errors;
for(const key15 of Object.keys(data68)){
if(!(((((key15 === "evidence") || (key15 === "kind")) || (key15 === "search")) || (key15 === "selected")) || (key15 === "status"))){
validate21.errors = [{instancePath:instancePath+"/regions",schemaPath:"#/$defs/RegionViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key15},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs189 === errors){
let data69 = data68.evidence;
const _errs190 = errors;
if(errors === _errs190){
if(typeof data69 === "string"){
if(func63(data69) > 8192){
validate21.errors = [{instancePath:instancePath+"/regions/evidence",schemaPath:"#/$defs/RegionViewSettings/properties/evidence/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/regions/evidence",schemaPath:"#/$defs/RegionViewSettings/properties/evidence/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid36 = _errs190 === errors;
if(valid36){
let data70 = data68.kind;
const _errs192 = errors;
if(errors === _errs192){
if(typeof data70 === "string"){
if(func63(data70) > 8192){
validate21.errors = [{instancePath:instancePath+"/regions/kind",schemaPath:"#/$defs/RegionViewSettings/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/regions/kind",schemaPath:"#/$defs/RegionViewSettings/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid36 = _errs192 === errors;
if(valid36){
let data71 = data68.search;
const _errs194 = errors;
if(errors === _errs194){
if(typeof data71 === "string"){
if(func63(data71) > 2048){
validate21.errors = [{instancePath:instancePath+"/regions/search",schemaPath:"#/$defs/RegionViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/regions/search",schemaPath:"#/$defs/RegionViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid36 = _errs194 === errors;
if(valid36){
let data72 = data68.selected;
const _errs196 = errors;
const _errs197 = errors;
let valid37 = false;
const _errs198 = errors;
if(errors === _errs198){
if(typeof data72 === "string"){
if(func63(data72) > 8192){
const err20 = {instancePath:instancePath+"/regions/selected",schemaPath:"#/$defs/RegionViewSettings/properties/selected/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
if(vErrors === null){
vErrors = [err20];
}
else {
vErrors.push(err20);
}
errors++;
}
}
else {
const err21 = {instancePath:instancePath+"/regions/selected",schemaPath:"#/$defs/RegionViewSettings/properties/selected/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err21];
}
else {
vErrors.push(err21);
}
errors++;
}
}
var _valid5 = _errs198 === errors;
valid37 = valid37 || _valid5;
const _errs200 = errors;
if(data72 !== null){
const err22 = {instancePath:instancePath+"/regions/selected",schemaPath:"#/$defs/RegionViewSettings/properties/selected/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err22];
}
else {
vErrors.push(err22);
}
errors++;
}
var _valid5 = _errs200 === errors;
valid37 = valid37 || _valid5;
if(!valid37){
const err23 = {instancePath:instancePath+"/regions/selected",schemaPath:"#/$defs/RegionViewSettings/properties/selected/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err23];
}
else {
vErrors.push(err23);
}
errors++;
validate21.errors = vErrors;
return false;
}
else {
errors = _errs197;
if(vErrors !== null){
if(_errs197){
vErrors.length = _errs197;
}
else {
vErrors = null;
}
}
}
var valid36 = _errs196 === errors;
if(valid36){
let data73 = data68.status;
const _errs202 = errors;
if(errors === _errs202){
if(typeof data73 === "string"){
if(func63(data73) > 8192){
validate21.errors = [{instancePath:instancePath+"/regions/status",schemaPath:"#/$defs/RegionViewSettings/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/regions/status",schemaPath:"#/$defs/RegionViewSettings/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid36 = _errs202 === errors;
}
}
}
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/regions",schemaPath:"#/$defs/RegionViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs186 === errors;
if(valid0){
let data74 = data.selected_federate;
const _errs204 = errors;
if(errors === _errs204){
if(typeof data74 === "string"){
if(func63(data74) > 8192){
validate21.errors = [{instancePath:instancePath+"/selected_federate",schemaPath:"#/properties/selected_federate/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/selected_federate",schemaPath:"#/properties/selected_federate/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs204 === errors;
if(valid0){
let data75 = data.synchronization;
const _errs206 = errors;
const _errs207 = errors;
if(errors === _errs207){
if(data75 && typeof data75 == "object" && !Array.isArray(data75)){
if(data75.kind === undefined){
data75.kind = "all";
}
if(data75.phase === undefined){
data75.phase = "all";
}
if(data75.scope === undefined){
data75.scope = "all";
}
if(data75.search === undefined){
data75.search = "";
}
if(data75.selected === undefined){
data75.selected = "all";
}
const _errs209 = errors;
for(const key16 of Object.keys(data75)){
if(!(((((key16 === "kind") || (key16 === "phase")) || (key16 === "scope")) || (key16 === "search")) || (key16 === "selected"))){
validate21.errors = [{instancePath:instancePath+"/synchronization",schemaPath:"#/$defs/CoordinationViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key16},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs209 === errors){
let data76 = data75.kind;
const _errs210 = errors;
if(errors === _errs210){
if(typeof data76 === "string"){
if(func63(data76) > 8192){
validate21.errors = [{instancePath:instancePath+"/synchronization/kind",schemaPath:"#/$defs/CoordinationViewSettings/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/synchronization/kind",schemaPath:"#/$defs/CoordinationViewSettings/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid39 = _errs210 === errors;
if(valid39){
let data77 = data75.phase;
const _errs212 = errors;
if(errors === _errs212){
if(typeof data77 === "string"){
if(func63(data77) > 8192){
validate21.errors = [{instancePath:instancePath+"/synchronization/phase",schemaPath:"#/$defs/CoordinationViewSettings/properties/phase/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/synchronization/phase",schemaPath:"#/$defs/CoordinationViewSettings/properties/phase/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid39 = _errs212 === errors;
if(valid39){
let data78 = data75.scope;
const _errs214 = errors;
if(errors === _errs214){
if(typeof data78 === "string"){
if(func63(data78) > 8192){
validate21.errors = [{instancePath:instancePath+"/synchronization/scope",schemaPath:"#/$defs/CoordinationViewSettings/properties/scope/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/synchronization/scope",schemaPath:"#/$defs/CoordinationViewSettings/properties/scope/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid39 = _errs214 === errors;
if(valid39){
let data79 = data75.search;
const _errs216 = errors;
if(errors === _errs216){
if(typeof data79 === "string"){
if(func63(data79) > 2048){
validate21.errors = [{instancePath:instancePath+"/synchronization/search",schemaPath:"#/$defs/CoordinationViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/synchronization/search",schemaPath:"#/$defs/CoordinationViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid39 = _errs216 === errors;
if(valid39){
let data80 = data75.selected;
const _errs218 = errors;
if(errors === _errs218){
if(typeof data80 === "string"){
if(func63(data80) > 8192){
validate21.errors = [{instancePath:instancePath+"/synchronization/selected",schemaPath:"#/$defs/CoordinationViewSettings/properties/selected/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/synchronization/selected",schemaPath:"#/$defs/CoordinationViewSettings/properties/selected/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid39 = _errs218 === errors;
}
}
}
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/synchronization",schemaPath:"#/$defs/CoordinationViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs206 === errors;
if(valid0){
let data81 = data.time;
const _errs220 = errors;
const _errs221 = errors;
if(errors === _errs221){
if(data81 && typeof data81 == "object" && !Array.isArray(data81)){
if(data81.grants_only === undefined){
data81.grants_only = false;
}
if(data81.selected === undefined){
data81.selected = null;
}
const _errs223 = errors;
for(const key17 of Object.keys(data81)){
if(!((key17 === "grants_only") || (key17 === "selected"))){
validate21.errors = [{instancePath:instancePath+"/time",schemaPath:"#/$defs/TimeViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key17},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs223 === errors){
const _errs224 = errors;
if(typeof data81.grants_only !== "boolean"){
validate21.errors = [{instancePath:instancePath+"/time/grants_only",schemaPath:"#/$defs/TimeViewSettings/properties/grants_only/type",keyword:"type",params:{type: "boolean"},message:"must be boolean"}];
return false;
}
var valid41 = _errs224 === errors;
if(valid41){
let data83 = data81.selected;
const _errs226 = errors;
const _errs227 = errors;
let valid42 = false;
const _errs228 = errors;
if(errors === _errs228){
if(typeof data83 === "string"){
if(func63(data83) > 8192){
const err24 = {instancePath:instancePath+"/time/selected",schemaPath:"#/$defs/TimeViewSettings/properties/selected/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
if(vErrors === null){
vErrors = [err24];
}
else {
vErrors.push(err24);
}
errors++;
}
}
else {
const err25 = {instancePath:instancePath+"/time/selected",schemaPath:"#/$defs/TimeViewSettings/properties/selected/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err25];
}
else {
vErrors.push(err25);
}
errors++;
}
}
var _valid6 = _errs228 === errors;
valid42 = valid42 || _valid6;
const _errs230 = errors;
if(data83 !== null){
const err26 = {instancePath:instancePath+"/time/selected",schemaPath:"#/$defs/TimeViewSettings/properties/selected/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err26];
}
else {
vErrors.push(err26);
}
errors++;
}
var _valid6 = _errs230 === errors;
valid42 = valid42 || _valid6;
if(!valid42){
const err27 = {instancePath:instancePath+"/time/selected",schemaPath:"#/$defs/TimeViewSettings/properties/selected/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err27];
}
else {
vErrors.push(err27);
}
errors++;
validate21.errors = vErrors;
return false;
}
else {
errors = _errs227;
if(vErrors !== null){
if(_errs227){
vErrors.length = _errs227;
}
else {
vErrors = null;
}
}
}
var valid41 = _errs226 === errors;
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/time",schemaPath:"#/$defs/TimeViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs220 === errors;
if(valid0){
let data84 = data.topology;
const _errs232 = errors;
const _errs233 = errors;
if(errors === _errs233){
if(data84 && typeof data84 == "object" && !Array.isArray(data84)){
if(data84.evidence === undefined){
data84.evidence = "all";
}
if(data84.focused === undefined){
data84.focused = "all";
}
if(data84.kind === undefined){
data84.kind = "all";
}
if(data84.search === undefined){
data84.search = "";
}
if(data84.source === undefined){
data84.source = "all";
}
if(data84.status === undefined){
data84.status = "all";
}
const _errs235 = errors;
for(const key18 of Object.keys(data84)){
if(!((((((key18 === "evidence") || (key18 === "focused")) || (key18 === "kind")) || (key18 === "search")) || (key18 === "source")) || (key18 === "status"))){
validate21.errors = [{instancePath:instancePath+"/topology",schemaPath:"#/$defs/TopologyViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key18},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs235 === errors){
let data85 = data84.evidence;
const _errs236 = errors;
if(errors === _errs236){
if(typeof data85 === "string"){
if(func63(data85) > 8192){
validate21.errors = [{instancePath:instancePath+"/topology/evidence",schemaPath:"#/$defs/TopologyViewSettings/properties/evidence/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/topology/evidence",schemaPath:"#/$defs/TopologyViewSettings/properties/evidence/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid44 = _errs236 === errors;
if(valid44){
let data86 = data84.focused;
const _errs238 = errors;
if(errors === _errs238){
if(typeof data86 === "string"){
if(func63(data86) > 8192){
validate21.errors = [{instancePath:instancePath+"/topology/focused",schemaPath:"#/$defs/TopologyViewSettings/properties/focused/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/topology/focused",schemaPath:"#/$defs/TopologyViewSettings/properties/focused/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid44 = _errs238 === errors;
if(valid44){
let data87 = data84.kind;
const _errs240 = errors;
if(errors === _errs240){
if(typeof data87 === "string"){
if(func63(data87) > 8192){
validate21.errors = [{instancePath:instancePath+"/topology/kind",schemaPath:"#/$defs/TopologyViewSettings/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/topology/kind",schemaPath:"#/$defs/TopologyViewSettings/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid44 = _errs240 === errors;
if(valid44){
let data88 = data84.search;
const _errs242 = errors;
if(errors === _errs242){
if(typeof data88 === "string"){
if(func63(data88) > 2048){
validate21.errors = [{instancePath:instancePath+"/topology/search",schemaPath:"#/$defs/TopologyViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/topology/search",schemaPath:"#/$defs/TopologyViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid44 = _errs242 === errors;
if(valid44){
let data89 = data84.source;
const _errs244 = errors;
if(errors === _errs244){
if(typeof data89 === "string"){
if(func63(data89) > 8192){
validate21.errors = [{instancePath:instancePath+"/topology/source",schemaPath:"#/$defs/TopologyViewSettings/properties/source/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/topology/source",schemaPath:"#/$defs/TopologyViewSettings/properties/source/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid44 = _errs244 === errors;
if(valid44){
let data90 = data84.status;
const _errs246 = errors;
if(errors === _errs246){
if(typeof data90 === "string"){
if(func63(data90) > 8192){
validate21.errors = [{instancePath:instancePath+"/topology/status",schemaPath:"#/$defs/TopologyViewSettings/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate21.errors = [{instancePath:instancePath+"/topology/status",schemaPath:"#/$defs/TopologyViewSettings/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid44 = _errs246 === errors;
}
}
}
}
}
}
}
else {
validate21.errors = [{instancePath:instancePath+"/topology",schemaPath:"#/$defs/TopologyViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs232 === errors;
if(valid0){
let data91 = data.view;
const _errs248 = errors;
if(typeof data91 !== "string"){
validate21.errors = [{instancePath:instancePath+"/view",schemaPath:"#/properties/view/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
if(!(((((((((((((((((((data91 === "federation") || (data91 === "federates")) || (data91 === "objects")) || (data91 === "declarations")) || (data91 === "topology")) || (data91 === "interactions")) || (data91 === "ownership")) || (data91 === "regions")) || (data91 === "mom")) || (data91 === "fom")) || (data91 === "time")) || (data91 === "synchronization")) || (data91 === "persistence")) || (data91 === "events")) || (data91 === "inspectors")) || (data91 === "recordings")) || (data91 === "alerts")) || (data91 === "actions")) || (data91 === "diagnostics"))){
validate21.errors = [{instancePath:instancePath+"/view",schemaPath:"#/properties/view/enum",keyword:"enum",params:{allowedValues: schema32.properties.view.enum},message:"must be equal to one of the allowed values"}];
return false;
}
var valid0 = _errs248 === errors;
if(valid0){
let data92 = data.window_seconds;
const _errs250 = errors;
if(!(((typeof data92 == "number") && (!(data92 % 1) && !isNaN(data92))) && (isFinite(data92)))){
validate21.errors = [{instancePath:instancePath+"/window_seconds",schemaPath:"#/properties/window_seconds/type",keyword:"type",params:{type: "integer"},message:"must be integer"}];
return false;
}
if(!((((data92 === 30) || (data92 === 300)) || (data92 === 900)) || (data92 === 3600))){
validate21.errors = [{instancePath:instancePath+"/window_seconds",schemaPath:"#/properties/window_seconds/enum",keyword:"enum",params:{allowedValues: schema32.properties.window_seconds.enum},message:"must be equal to one of the allowed values"}];
return false;
}
var valid0 = _errs250 === errors;
}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
else {
validate21.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate21.errors = vErrors;
return errors === 0;
}
validate21.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};


function validate20(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
/*# sourceURL="urn:umbra:observer:workspace:v1" */;
let vErrors = null;
let errors = 0;
const evaluated0 = validate20.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.settings === undefined){
data.settings = {"alerts":{"search":"","status":"all"},"declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"events":{"family":"","federate":"","search":"","source":"","type":""},"federate_declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"federate_events":{"family":"","federate":"","search":"","source":"","type":""},"federate_time":{"grants_only":false,"selected":null},"federation":{"distribution":"event_types"},"fom":{"kind":"all","model":"all","search":"","selected":null,"status":"all"},"interactions":{"order":"all","search":"","status":"all"},"mom":{"counter":null,"scope":"all","search":"","service_status":"all"},"object_charts":[],"objects":{"scope":null,"search":"","selected":"","status":"all"},"ownership":{"owner_state":"all","request_status":"all","scope":"all","search":""},"persistence":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"regions":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"},"selected_federate":"","synchronization":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"time":{"grants_only":false,"selected":null},"topology":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"},"view":"federation","window_seconds":300};
}
let missing0;
if((((data.workspace_version === undefined) || (!(func0.call(data, "workspace_version")))) && (missing0 = "workspace_version")) || (((data.name === undefined) || (!(func0.call(data, "name")))) && (missing0 = "name"))){
validate20.errors = [{instancePath,schemaPath:"#/required",keyword:"required",params:{missingProperty: missing0},message:"must have required property '"+missing0+"'"}];
return false;
}
else {
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((key0 === "name") || (key0 === "settings")) || (key0 === "workspace_version"))){
validate20.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
if(data.name !== undefined && func0.call(data, "name")){
let data0 = data.name;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 128){
validate20.errors = [{instancePath:instancePath+"/name",schemaPath:"#/properties/name/maxLength",keyword:"maxLength",params:{limit: 128},message:"must NOT have more than 128 characters"}];
return false;
}
else {
if(func63(data0) < 1){
validate20.errors = [{instancePath:instancePath+"/name",schemaPath:"#/properties/name/minLength",keyword:"minLength",params:{limit: 1},message:"must NOT have fewer than 1 characters"}];
return false;
}
else {
if(!pattern4.test(data0)){
validate20.errors = [{instancePath:instancePath+"/name",schemaPath:"#/properties/name/pattern",keyword:"pattern",params:{pattern: "\\S"},message:"must match pattern \""+"\\S"+"\""}];
return false;
}
}
}
}
else {
validate20.errors = [{instancePath:instancePath+"/name",schemaPath:"#/properties/name/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
}
else {
var valid0 = true;
}
if(valid0){
const _errs4 = errors;
if(!(validate21(data.settings, {instancePath:instancePath+"/settings",parentData:data,parentDataProperty:"settings",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate21.errors : vErrors.concat(validate21.errors);
errors = vErrors.length;
}
var valid0 = _errs4 === errors;
if(valid0){
if(data.workspace_version !== undefined && func0.call(data, "workspace_version")){
let data2 = data.workspace_version;
const _errs5 = errors;
if(typeof data2 !== "string"){
validate20.errors = [{instancePath:instancePath+"/workspace_version",schemaPath:"#/properties/workspace_version/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
if("umbra-observer-workspace-v1" !== data2){
validate20.errors = [{instancePath:instancePath+"/workspace_version",schemaPath:"#/properties/workspace_version/const",keyword:"const",params:{allowedValue: "umbra-observer-workspace-v1"},message:"must be equal to constant"}];
return false;
}
var valid0 = _errs5 === errors;
}
else {
var valid0 = true;
}
}
}
}
}
}
else {
validate20.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate20.errors = vErrors;
return errors === 0;
}
validate20.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

exports.validateLibrary = validate23;
const schema51 = {"$defs":{"AlertViewSettings":{"additionalProperties":false,"properties":{"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"AlertViewSettings","type":"object"},"CoordinationViewSettings":{"additionalProperties":false,"properties":{"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"phase":{"default":"all","maxLength":8192,"title":"Phase","type":"string"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"default":"all","maxLength":8192,"title":"Selected","type":"string"}},"title":"CoordinationViewSettings","type":"object"},"DeclarationViewSettings":{"additionalProperties":false,"properties":{"direction":{"default":"all","maxLength":8192,"title":"Direction","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"perspective":{"default":"all","maxLength":8192,"title":"Perspective","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"DeclarationViewSettings","type":"object"},"EventViewSettings":{"additionalProperties":false,"properties":{"family":{"default":"","maxLength":8192,"title":"Family","type":"string"},"federate":{"default":"","maxLength":8192,"title":"Federate","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"source":{"default":"","maxLength":8192,"title":"Source","type":"string"},"type":{"default":"","maxLength":8192,"title":"Type","type":"string"}},"title":"EventViewSettings","type":"object"},"FederationViewSettings":{"additionalProperties":false,"properties":{"distribution":{"default":"event_types","enum":["event_types","sources","families"],"title":"Distribution","type":"string"}},"title":"FederationViewSettings","type":"object"},"FomViewSettings":{"additionalProperties":false,"properties":{"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"model":{"default":"all","maxLength":8192,"title":"Model","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"FomViewSettings","type":"object"},"InteractionViewSettings":{"additionalProperties":false,"properties":{"order":{"default":"all","maxLength":8192,"title":"Order","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"InteractionViewSettings","type":"object"},"MomViewSettings":{"additionalProperties":false,"properties":{"counter":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Counter"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"service_status":{"default":"all","maxLength":8192,"title":"Service Status","type":"string"}},"title":"MomViewSettings","type":"object"},"ObjectChartChoice":{"additionalProperties":false,"properties":{"attribute":{"maxLength":8192,"minLength":1,"title":"Attribute","type":"string"},"object_key":{"maxLength":8192,"minLength":1,"title":"Object Key","type":"string"},"scope":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Scope"}},"required":["object_key","attribute"],"title":"ObjectChartChoice","type":"object"},"ObjectViewSettings":{"additionalProperties":false,"properties":{"scope":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Scope"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"default":"","maxLength":8192,"title":"Selected","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"ObjectViewSettings","type":"object"},"OwnershipViewSettings":{"additionalProperties":false,"properties":{"owner_state":{"default":"all","maxLength":8192,"title":"Owner State","type":"string"},"request_status":{"default":"all","maxLength":8192,"title":"Request Status","type":"string"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"}},"title":"OwnershipViewSettings","type":"object"},"RegionViewSettings":{"additionalProperties":false,"properties":{"evidence":{"default":"all","maxLength":8192,"title":"Evidence","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"RegionViewSettings","type":"object"},"TimeViewSettings":{"additionalProperties":false,"properties":{"grants_only":{"default":false,"title":"Grants Only","type":"boolean"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"}},"title":"TimeViewSettings","type":"object"},"TopologyViewSettings":{"additionalProperties":false,"properties":{"evidence":{"default":"all","maxLength":8192,"title":"Evidence","type":"string"},"focused":{"default":"all","maxLength":8192,"title":"Focused","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"source":{"default":"all","maxLength":8192,"title":"Source","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"TopologyViewSettings","type":"object"},"WorkspaceDocument":{"additionalProperties":false,"properties":{"name":{"maxLength":128,"minLength":1,"pattern":"\\S","title":"Name","type":"string"},"settings":{"$ref":"#/$defs/WorkspaceSettings","default":{"alerts":{"search":"","status":"all"},"declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"events":{"family":"","federate":"","search":"","source":"","type":""},"federate_declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"federate_events":{"family":"","federate":"","search":"","source":"","type":""},"federate_time":{"grants_only":false,"selected":null},"federation":{"distribution":"event_types"},"fom":{"kind":"all","model":"all","search":"","selected":null,"status":"all"},"interactions":{"order":"all","search":"","status":"all"},"mom":{"counter":null,"scope":"all","search":"","service_status":"all"},"object_charts":[],"objects":{"scope":null,"search":"","selected":"","status":"all"},"ownership":{"owner_state":"all","request_status":"all","scope":"all","search":""},"persistence":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"regions":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"},"selected_federate":"","synchronization":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"time":{"grants_only":false,"selected":null},"topology":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"},"view":"federation","window_seconds":300}},"workspace_version":{"const":"umbra-observer-workspace-v1","title":"Workspace Version","type":"string"}},"required":["workspace_version","name"],"title":"WorkspaceDocument","type":"object"},"WorkspaceSettings":{"additionalProperties":false,"properties":{"alerts":{"$ref":"#/$defs/AlertViewSettings","default":{"search":"","status":"all"}},"declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"federate_declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"federate_events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"federate_time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}},"federation":{"$ref":"#/$defs/FederationViewSettings","default":{"distribution":"event_types"}},"fom":{"$ref":"#/$defs/FomViewSettings","default":{"kind":"all","model":"all","search":"","selected":null,"status":"all"}},"interactions":{"$ref":"#/$defs/InteractionViewSettings","default":{"order":"all","search":"","status":"all"}},"mom":{"$ref":"#/$defs/MomViewSettings","default":{"counter":null,"scope":"all","search":"","service_status":"all"}},"object_charts":{"default":[],"items":{"$ref":"#/$defs/ObjectChartChoice"},"maxItems":64,"title":"Object Charts","type":"array"},"objects":{"$ref":"#/$defs/ObjectViewSettings","default":{"scope":null,"search":"","selected":"","status":"all"}},"ownership":{"$ref":"#/$defs/OwnershipViewSettings","default":{"owner_state":"all","request_status":"all","scope":"all","search":""}},"persistence":{"$ref":"#/$defs/CoordinationViewSettings","default":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"}},"regions":{"$ref":"#/$defs/RegionViewSettings","default":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"}},"selected_federate":{"default":"","maxLength":8192,"title":"Selected Federate","type":"string"},"synchronization":{"$ref":"#/$defs/CoordinationViewSettings","default":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"}},"time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}},"topology":{"$ref":"#/$defs/TopologyViewSettings","default":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"}},"view":{"default":"federation","enum":["federation","federates","objects","declarations","topology","interactions","ownership","regions","mom","fom","time","synchronization","persistence","events","inspectors","recordings","alerts","actions","diagnostics"],"title":"View","type":"string"},"window_seconds":{"default":300,"enum":[30,300,900,3600],"title":"Window Seconds","type":"integer"}},"title":"WorkspaceSettings","type":"object"}},"$id":"urn:umbra:observer:workspace-library:v1","$schema":"https://json-schema.org/draft/2020-12/schema","additionalProperties":false,"properties":{"current":{"$ref":"#/$defs/WorkspaceSettings","default":{"alerts":{"search":"","status":"all"},"declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"events":{"family":"","federate":"","search":"","source":"","type":""},"federate_declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"federate_events":{"family":"","federate":"","search":"","source":"","type":""},"federate_time":{"grants_only":false,"selected":null},"federation":{"distribution":"event_types"},"fom":{"kind":"all","model":"all","search":"","selected":null,"status":"all"},"interactions":{"order":"all","search":"","status":"all"},"mom":{"counter":null,"scope":"all","search":"","service_status":"all"},"object_charts":[],"objects":{"scope":null,"search":"","selected":"","status":"all"},"ownership":{"owner_state":"all","request_status":"all","scope":"all","search":""},"persistence":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"regions":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"},"selected_federate":"","synchronization":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"time":{"grants_only":false,"selected":null},"topology":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"},"view":"federation","window_seconds":300}},"library_version":{"const":"umbra-observer-workspace-library-v1","title":"Library Version","type":"string"},"saved":{"default":[],"items":{"$ref":"#/$defs/WorkspaceDocument"},"maxItems":12,"title":"Saved","type":"array"}},"required":["library_version"],"title":"WorkspaceLibrary","type":"object"};
const schema52 = {"additionalProperties":false,"properties":{"alerts":{"$ref":"#/$defs/AlertViewSettings","default":{"search":"","status":"all"}},"declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"federate_declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"federate_events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"federate_time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}},"federation":{"$ref":"#/$defs/FederationViewSettings","default":{"distribution":"event_types"}},"fom":{"$ref":"#/$defs/FomViewSettings","default":{"kind":"all","model":"all","search":"","selected":null,"status":"all"}},"interactions":{"$ref":"#/$defs/InteractionViewSettings","default":{"order":"all","search":"","status":"all"}},"mom":{"$ref":"#/$defs/MomViewSettings","default":{"counter":null,"scope":"all","search":"","service_status":"all"}},"object_charts":{"default":[],"items":{"$ref":"#/$defs/ObjectChartChoice"},"maxItems":64,"title":"Object Charts","type":"array"},"objects":{"$ref":"#/$defs/ObjectViewSettings","default":{"scope":null,"search":"","selected":"","status":"all"}},"ownership":{"$ref":"#/$defs/OwnershipViewSettings","default":{"owner_state":"all","request_status":"all","scope":"all","search":""}},"persistence":{"$ref":"#/$defs/CoordinationViewSettings","default":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"}},"regions":{"$ref":"#/$defs/RegionViewSettings","default":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"}},"selected_federate":{"default":"","maxLength":8192,"title":"Selected Federate","type":"string"},"synchronization":{"$ref":"#/$defs/CoordinationViewSettings","default":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"}},"time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}},"topology":{"$ref":"#/$defs/TopologyViewSettings","default":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"}},"view":{"default":"federation","enum":["federation","federates","objects","declarations","topology","interactions","ownership","regions","mom","fom","time","synchronization","persistence","events","inspectors","recordings","alerts","actions","diagnostics"],"title":"View","type":"string"},"window_seconds":{"default":300,"enum":[30,300,900,3600],"title":"Window Seconds","type":"integer"}},"title":"WorkspaceSettings","type":"object"};
const schema53 = {"additionalProperties":false,"properties":{"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"AlertViewSettings","type":"object"};
const schema54 = {"additionalProperties":false,"properties":{"direction":{"default":"all","maxLength":8192,"title":"Direction","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"perspective":{"default":"all","maxLength":8192,"title":"Perspective","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"DeclarationViewSettings","type":"object"};
const schema55 = {"additionalProperties":false,"properties":{"family":{"default":"","maxLength":8192,"title":"Family","type":"string"},"federate":{"default":"","maxLength":8192,"title":"Federate","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"source":{"default":"","maxLength":8192,"title":"Source","type":"string"},"type":{"default":"","maxLength":8192,"title":"Type","type":"string"}},"title":"EventViewSettings","type":"object"};
const schema58 = {"additionalProperties":false,"properties":{"grants_only":{"default":false,"title":"Grants Only","type":"boolean"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"}},"title":"TimeViewSettings","type":"object"};
const schema59 = {"additionalProperties":false,"properties":{"distribution":{"default":"event_types","enum":["event_types","sources","families"],"title":"Distribution","type":"string"}},"title":"FederationViewSettings","type":"object"};
const schema60 = {"additionalProperties":false,"properties":{"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"model":{"default":"all","maxLength":8192,"title":"Model","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"FomViewSettings","type":"object"};
const schema61 = {"additionalProperties":false,"properties":{"order":{"default":"all","maxLength":8192,"title":"Order","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"InteractionViewSettings","type":"object"};
const schema62 = {"additionalProperties":false,"properties":{"counter":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Counter"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"service_status":{"default":"all","maxLength":8192,"title":"Service Status","type":"string"}},"title":"MomViewSettings","type":"object"};
const schema63 = {"additionalProperties":false,"properties":{"attribute":{"maxLength":8192,"minLength":1,"title":"Attribute","type":"string"},"object_key":{"maxLength":8192,"minLength":1,"title":"Object Key","type":"string"},"scope":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Scope"}},"required":["object_key","attribute"],"title":"ObjectChartChoice","type":"object"};
const schema64 = {"additionalProperties":false,"properties":{"scope":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Scope"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"default":"","maxLength":8192,"title":"Selected","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"ObjectViewSettings","type":"object"};
const schema65 = {"additionalProperties":false,"properties":{"owner_state":{"default":"all","maxLength":8192,"title":"Owner State","type":"string"},"request_status":{"default":"all","maxLength":8192,"title":"Request Status","type":"string"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"}},"title":"OwnershipViewSettings","type":"object"};
const schema66 = {"additionalProperties":false,"properties":{"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"phase":{"default":"all","maxLength":8192,"title":"Phase","type":"string"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"default":"all","maxLength":8192,"title":"Selected","type":"string"}},"title":"CoordinationViewSettings","type":"object"};
const schema67 = {"additionalProperties":false,"properties":{"evidence":{"default":"all","maxLength":8192,"title":"Evidence","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"RegionViewSettings","type":"object"};
const schema70 = {"additionalProperties":false,"properties":{"evidence":{"default":"all","maxLength":8192,"title":"Evidence","type":"string"},"focused":{"default":"all","maxLength":8192,"title":"Focused","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"source":{"default":"all","maxLength":8192,"title":"Source","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"TopologyViewSettings","type":"object"};

function validate24(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate24.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.alerts === undefined){
data.alerts = {"search":"","status":"all"};
}
if(data.declarations === undefined){
data.declarations = {"direction":"all","kind":"all","perspective":"all","search":"","status":"all"};
}
if(data.events === undefined){
data.events = {"family":"","federate":"","search":"","source":"","type":""};
}
if(data.federate_declarations === undefined){
data.federate_declarations = {"direction":"all","kind":"all","perspective":"all","search":"","status":"all"};
}
if(data.federate_events === undefined){
data.federate_events = {"family":"","federate":"","search":"","source":"","type":""};
}
if(data.federate_time === undefined){
data.federate_time = {"grants_only":false,"selected":null};
}
if(data.federation === undefined){
data.federation = {"distribution":"event_types"};
}
if(data.fom === undefined){
data.fom = {"kind":"all","model":"all","search":"","selected":null,"status":"all"};
}
if(data.interactions === undefined){
data.interactions = {"order":"all","search":"","status":"all"};
}
if(data.mom === undefined){
data.mom = {"counter":null,"scope":"all","search":"","service_status":"all"};
}
if(data.object_charts === undefined){
data.object_charts = [];
}
if(data.objects === undefined){
data.objects = {"scope":null,"search":"","selected":"","status":"all"};
}
if(data.ownership === undefined){
data.ownership = {"owner_state":"all","request_status":"all","scope":"all","search":""};
}
if(data.persistence === undefined){
data.persistence = {"kind":"all","phase":"all","scope":"all","search":"","selected":"all"};
}
if(data.regions === undefined){
data.regions = {"evidence":"all","kind":"all","search":"","selected":null,"status":"all"};
}
if(data.selected_federate === undefined){
data.selected_federate = "";
}
if(data.synchronization === undefined){
data.synchronization = {"kind":"all","phase":"all","scope":"all","search":"","selected":"all"};
}
if(data.time === undefined){
data.time = {"grants_only":false,"selected":null};
}
if(data.topology === undefined){
data.topology = {"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"};
}
if(data.view === undefined){
data.view = "federation";
}
if(data.window_seconds === undefined){
data.window_seconds = 300;
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(func0.call(schema52.properties, key0))){
validate24.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.alerts;
const _errs2 = errors;
const _errs3 = errors;
if(errors === _errs3){
if(data0 && typeof data0 == "object" && !Array.isArray(data0)){
if(data0.search === undefined){
data0.search = "";
}
if(data0.status === undefined){
data0.status = "all";
}
const _errs5 = errors;
for(const key1 of Object.keys(data0)){
if(!((key1 === "search") || (key1 === "status"))){
validate24.errors = [{instancePath:instancePath+"/alerts",schemaPath:"#/$defs/AlertViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key1},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs5 === errors){
let data1 = data0.search;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data1 === "string"){
if(func63(data1) > 2048){
validate24.errors = [{instancePath:instancePath+"/alerts/search",schemaPath:"#/$defs/AlertViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/alerts/search",schemaPath:"#/$defs/AlertViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid2 = _errs6 === errors;
if(valid2){
let data2 = data0.status;
const _errs8 = errors;
if(errors === _errs8){
if(typeof data2 === "string"){
if(func63(data2) > 8192){
validate24.errors = [{instancePath:instancePath+"/alerts/status",schemaPath:"#/$defs/AlertViewSettings/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/alerts/status",schemaPath:"#/$defs/AlertViewSettings/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid2 = _errs8 === errors;
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/alerts",schemaPath:"#/$defs/AlertViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data3 = data.declarations;
const _errs10 = errors;
const _errs11 = errors;
if(errors === _errs11){
if(data3 && typeof data3 == "object" && !Array.isArray(data3)){
if(data3.direction === undefined){
data3.direction = "all";
}
if(data3.kind === undefined){
data3.kind = "all";
}
if(data3.perspective === undefined){
data3.perspective = "all";
}
if(data3.search === undefined){
data3.search = "";
}
if(data3.status === undefined){
data3.status = "all";
}
const _errs13 = errors;
for(const key2 of Object.keys(data3)){
if(!(((((key2 === "direction") || (key2 === "kind")) || (key2 === "perspective")) || (key2 === "search")) || (key2 === "status"))){
validate24.errors = [{instancePath:instancePath+"/declarations",schemaPath:"#/$defs/DeclarationViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key2},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs13 === errors){
let data4 = data3.direction;
const _errs14 = errors;
if(errors === _errs14){
if(typeof data4 === "string"){
if(func63(data4) > 8192){
validate24.errors = [{instancePath:instancePath+"/declarations/direction",schemaPath:"#/$defs/DeclarationViewSettings/properties/direction/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/declarations/direction",schemaPath:"#/$defs/DeclarationViewSettings/properties/direction/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid4 = _errs14 === errors;
if(valid4){
let data5 = data3.kind;
const _errs16 = errors;
if(errors === _errs16){
if(typeof data5 === "string"){
if(func63(data5) > 8192){
validate24.errors = [{instancePath:instancePath+"/declarations/kind",schemaPath:"#/$defs/DeclarationViewSettings/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/declarations/kind",schemaPath:"#/$defs/DeclarationViewSettings/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid4 = _errs16 === errors;
if(valid4){
let data6 = data3.perspective;
const _errs18 = errors;
if(errors === _errs18){
if(typeof data6 === "string"){
if(func63(data6) > 8192){
validate24.errors = [{instancePath:instancePath+"/declarations/perspective",schemaPath:"#/$defs/DeclarationViewSettings/properties/perspective/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/declarations/perspective",schemaPath:"#/$defs/DeclarationViewSettings/properties/perspective/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid4 = _errs18 === errors;
if(valid4){
let data7 = data3.search;
const _errs20 = errors;
if(errors === _errs20){
if(typeof data7 === "string"){
if(func63(data7) > 2048){
validate24.errors = [{instancePath:instancePath+"/declarations/search",schemaPath:"#/$defs/DeclarationViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/declarations/search",schemaPath:"#/$defs/DeclarationViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid4 = _errs20 === errors;
if(valid4){
let data8 = data3.status;
const _errs22 = errors;
if(errors === _errs22){
if(typeof data8 === "string"){
if(func63(data8) > 8192){
validate24.errors = [{instancePath:instancePath+"/declarations/status",schemaPath:"#/$defs/DeclarationViewSettings/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/declarations/status",schemaPath:"#/$defs/DeclarationViewSettings/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid4 = _errs22 === errors;
}
}
}
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/declarations",schemaPath:"#/$defs/DeclarationViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs10 === errors;
if(valid0){
let data9 = data.events;
const _errs24 = errors;
const _errs25 = errors;
if(errors === _errs25){
if(data9 && typeof data9 == "object" && !Array.isArray(data9)){
if(data9.family === undefined){
data9.family = "";
}
if(data9.federate === undefined){
data9.federate = "";
}
if(data9.search === undefined){
data9.search = "";
}
if(data9.source === undefined){
data9.source = "";
}
if(data9.type === undefined){
data9.type = "";
}
const _errs27 = errors;
for(const key3 of Object.keys(data9)){
if(!(((((key3 === "family") || (key3 === "federate")) || (key3 === "search")) || (key3 === "source")) || (key3 === "type"))){
validate24.errors = [{instancePath:instancePath+"/events",schemaPath:"#/$defs/EventViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key3},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs27 === errors){
let data10 = data9.family;
const _errs28 = errors;
if(errors === _errs28){
if(typeof data10 === "string"){
if(func63(data10) > 8192){
validate24.errors = [{instancePath:instancePath+"/events/family",schemaPath:"#/$defs/EventViewSettings/properties/family/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/events/family",schemaPath:"#/$defs/EventViewSettings/properties/family/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid6 = _errs28 === errors;
if(valid6){
let data11 = data9.federate;
const _errs30 = errors;
if(errors === _errs30){
if(typeof data11 === "string"){
if(func63(data11) > 8192){
validate24.errors = [{instancePath:instancePath+"/events/federate",schemaPath:"#/$defs/EventViewSettings/properties/federate/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/events/federate",schemaPath:"#/$defs/EventViewSettings/properties/federate/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid6 = _errs30 === errors;
if(valid6){
let data12 = data9.search;
const _errs32 = errors;
if(errors === _errs32){
if(typeof data12 === "string"){
if(func63(data12) > 2048){
validate24.errors = [{instancePath:instancePath+"/events/search",schemaPath:"#/$defs/EventViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/events/search",schemaPath:"#/$defs/EventViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid6 = _errs32 === errors;
if(valid6){
let data13 = data9.source;
const _errs34 = errors;
if(errors === _errs34){
if(typeof data13 === "string"){
if(func63(data13) > 8192){
validate24.errors = [{instancePath:instancePath+"/events/source",schemaPath:"#/$defs/EventViewSettings/properties/source/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/events/source",schemaPath:"#/$defs/EventViewSettings/properties/source/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid6 = _errs34 === errors;
if(valid6){
let data14 = data9.type;
const _errs36 = errors;
if(errors === _errs36){
if(typeof data14 === "string"){
if(func63(data14) > 8192){
validate24.errors = [{instancePath:instancePath+"/events/type",schemaPath:"#/$defs/EventViewSettings/properties/type/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/events/type",schemaPath:"#/$defs/EventViewSettings/properties/type/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid6 = _errs36 === errors;
}
}
}
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/events",schemaPath:"#/$defs/EventViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs24 === errors;
if(valid0){
let data15 = data.federate_declarations;
const _errs38 = errors;
const _errs39 = errors;
if(errors === _errs39){
if(data15 && typeof data15 == "object" && !Array.isArray(data15)){
if(data15.direction === undefined){
data15.direction = "all";
}
if(data15.kind === undefined){
data15.kind = "all";
}
if(data15.perspective === undefined){
data15.perspective = "all";
}
if(data15.search === undefined){
data15.search = "";
}
if(data15.status === undefined){
data15.status = "all";
}
const _errs41 = errors;
for(const key4 of Object.keys(data15)){
if(!(((((key4 === "direction") || (key4 === "kind")) || (key4 === "perspective")) || (key4 === "search")) || (key4 === "status"))){
validate24.errors = [{instancePath:instancePath+"/federate_declarations",schemaPath:"#/$defs/DeclarationViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key4},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs41 === errors){
let data16 = data15.direction;
const _errs42 = errors;
if(errors === _errs42){
if(typeof data16 === "string"){
if(func63(data16) > 8192){
validate24.errors = [{instancePath:instancePath+"/federate_declarations/direction",schemaPath:"#/$defs/DeclarationViewSettings/properties/direction/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/federate_declarations/direction",schemaPath:"#/$defs/DeclarationViewSettings/properties/direction/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid8 = _errs42 === errors;
if(valid8){
let data17 = data15.kind;
const _errs44 = errors;
if(errors === _errs44){
if(typeof data17 === "string"){
if(func63(data17) > 8192){
validate24.errors = [{instancePath:instancePath+"/federate_declarations/kind",schemaPath:"#/$defs/DeclarationViewSettings/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/federate_declarations/kind",schemaPath:"#/$defs/DeclarationViewSettings/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid8 = _errs44 === errors;
if(valid8){
let data18 = data15.perspective;
const _errs46 = errors;
if(errors === _errs46){
if(typeof data18 === "string"){
if(func63(data18) > 8192){
validate24.errors = [{instancePath:instancePath+"/federate_declarations/perspective",schemaPath:"#/$defs/DeclarationViewSettings/properties/perspective/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/federate_declarations/perspective",schemaPath:"#/$defs/DeclarationViewSettings/properties/perspective/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid8 = _errs46 === errors;
if(valid8){
let data19 = data15.search;
const _errs48 = errors;
if(errors === _errs48){
if(typeof data19 === "string"){
if(func63(data19) > 2048){
validate24.errors = [{instancePath:instancePath+"/federate_declarations/search",schemaPath:"#/$defs/DeclarationViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/federate_declarations/search",schemaPath:"#/$defs/DeclarationViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid8 = _errs48 === errors;
if(valid8){
let data20 = data15.status;
const _errs50 = errors;
if(errors === _errs50){
if(typeof data20 === "string"){
if(func63(data20) > 8192){
validate24.errors = [{instancePath:instancePath+"/federate_declarations/status",schemaPath:"#/$defs/DeclarationViewSettings/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/federate_declarations/status",schemaPath:"#/$defs/DeclarationViewSettings/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid8 = _errs50 === errors;
}
}
}
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/federate_declarations",schemaPath:"#/$defs/DeclarationViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs38 === errors;
if(valid0){
let data21 = data.federate_events;
const _errs52 = errors;
const _errs53 = errors;
if(errors === _errs53){
if(data21 && typeof data21 == "object" && !Array.isArray(data21)){
if(data21.family === undefined){
data21.family = "";
}
if(data21.federate === undefined){
data21.federate = "";
}
if(data21.search === undefined){
data21.search = "";
}
if(data21.source === undefined){
data21.source = "";
}
if(data21.type === undefined){
data21.type = "";
}
const _errs55 = errors;
for(const key5 of Object.keys(data21)){
if(!(((((key5 === "family") || (key5 === "federate")) || (key5 === "search")) || (key5 === "source")) || (key5 === "type"))){
validate24.errors = [{instancePath:instancePath+"/federate_events",schemaPath:"#/$defs/EventViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key5},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs55 === errors){
let data22 = data21.family;
const _errs56 = errors;
if(errors === _errs56){
if(typeof data22 === "string"){
if(func63(data22) > 8192){
validate24.errors = [{instancePath:instancePath+"/federate_events/family",schemaPath:"#/$defs/EventViewSettings/properties/family/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/federate_events/family",schemaPath:"#/$defs/EventViewSettings/properties/family/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid10 = _errs56 === errors;
if(valid10){
let data23 = data21.federate;
const _errs58 = errors;
if(errors === _errs58){
if(typeof data23 === "string"){
if(func63(data23) > 8192){
validate24.errors = [{instancePath:instancePath+"/federate_events/federate",schemaPath:"#/$defs/EventViewSettings/properties/federate/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/federate_events/federate",schemaPath:"#/$defs/EventViewSettings/properties/federate/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid10 = _errs58 === errors;
if(valid10){
let data24 = data21.search;
const _errs60 = errors;
if(errors === _errs60){
if(typeof data24 === "string"){
if(func63(data24) > 2048){
validate24.errors = [{instancePath:instancePath+"/federate_events/search",schemaPath:"#/$defs/EventViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/federate_events/search",schemaPath:"#/$defs/EventViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid10 = _errs60 === errors;
if(valid10){
let data25 = data21.source;
const _errs62 = errors;
if(errors === _errs62){
if(typeof data25 === "string"){
if(func63(data25) > 8192){
validate24.errors = [{instancePath:instancePath+"/federate_events/source",schemaPath:"#/$defs/EventViewSettings/properties/source/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/federate_events/source",schemaPath:"#/$defs/EventViewSettings/properties/source/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid10 = _errs62 === errors;
if(valid10){
let data26 = data21.type;
const _errs64 = errors;
if(errors === _errs64){
if(typeof data26 === "string"){
if(func63(data26) > 8192){
validate24.errors = [{instancePath:instancePath+"/federate_events/type",schemaPath:"#/$defs/EventViewSettings/properties/type/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/federate_events/type",schemaPath:"#/$defs/EventViewSettings/properties/type/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid10 = _errs64 === errors;
}
}
}
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/federate_events",schemaPath:"#/$defs/EventViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs52 === errors;
if(valid0){
let data27 = data.federate_time;
const _errs66 = errors;
const _errs67 = errors;
if(errors === _errs67){
if(data27 && typeof data27 == "object" && !Array.isArray(data27)){
if(data27.grants_only === undefined){
data27.grants_only = false;
}
if(data27.selected === undefined){
data27.selected = null;
}
const _errs69 = errors;
for(const key6 of Object.keys(data27)){
if(!((key6 === "grants_only") || (key6 === "selected"))){
validate24.errors = [{instancePath:instancePath+"/federate_time",schemaPath:"#/$defs/TimeViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key6},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs69 === errors){
const _errs70 = errors;
if(typeof data27.grants_only !== "boolean"){
validate24.errors = [{instancePath:instancePath+"/federate_time/grants_only",schemaPath:"#/$defs/TimeViewSettings/properties/grants_only/type",keyword:"type",params:{type: "boolean"},message:"must be boolean"}];
return false;
}
var valid12 = _errs70 === errors;
if(valid12){
let data29 = data27.selected;
const _errs72 = errors;
const _errs73 = errors;
let valid13 = false;
const _errs74 = errors;
if(errors === _errs74){
if(typeof data29 === "string"){
if(func63(data29) > 8192){
const err0 = {instancePath:instancePath+"/federate_time/selected",schemaPath:"#/$defs/TimeViewSettings/properties/selected/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
if(vErrors === null){
vErrors = [err0];
}
else {
vErrors.push(err0);
}
errors++;
}
}
else {
const err1 = {instancePath:instancePath+"/federate_time/selected",schemaPath:"#/$defs/TimeViewSettings/properties/selected/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err1];
}
else {
vErrors.push(err1);
}
errors++;
}
}
var _valid0 = _errs74 === errors;
valid13 = valid13 || _valid0;
const _errs76 = errors;
if(data29 !== null){
const err2 = {instancePath:instancePath+"/federate_time/selected",schemaPath:"#/$defs/TimeViewSettings/properties/selected/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err2];
}
else {
vErrors.push(err2);
}
errors++;
}
var _valid0 = _errs76 === errors;
valid13 = valid13 || _valid0;
if(!valid13){
const err3 = {instancePath:instancePath+"/federate_time/selected",schemaPath:"#/$defs/TimeViewSettings/properties/selected/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err3];
}
else {
vErrors.push(err3);
}
errors++;
validate24.errors = vErrors;
return false;
}
else {
errors = _errs73;
if(vErrors !== null){
if(_errs73){
vErrors.length = _errs73;
}
else {
vErrors = null;
}
}
}
var valid12 = _errs72 === errors;
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/federate_time",schemaPath:"#/$defs/TimeViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs66 === errors;
if(valid0){
let data30 = data.federation;
const _errs78 = errors;
const _errs79 = errors;
if(errors === _errs79){
if(data30 && typeof data30 == "object" && !Array.isArray(data30)){
if(data30.distribution === undefined){
data30.distribution = "event_types";
}
const _errs81 = errors;
for(const key7 of Object.keys(data30)){
if(!(key7 === "distribution")){
validate24.errors = [{instancePath:instancePath+"/federation",schemaPath:"#/$defs/FederationViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key7},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs81 === errors){
let data31 = data30.distribution;
if(typeof data31 !== "string"){
validate24.errors = [{instancePath:instancePath+"/federation/distribution",schemaPath:"#/$defs/FederationViewSettings/properties/distribution/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
if(!(((data31 === "event_types") || (data31 === "sources")) || (data31 === "families"))){
validate24.errors = [{instancePath:instancePath+"/federation/distribution",schemaPath:"#/$defs/FederationViewSettings/properties/distribution/enum",keyword:"enum",params:{allowedValues: schema59.properties.distribution.enum},message:"must be equal to one of the allowed values"}];
return false;
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/federation",schemaPath:"#/$defs/FederationViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs78 === errors;
if(valid0){
let data32 = data.fom;
const _errs84 = errors;
const _errs85 = errors;
if(errors === _errs85){
if(data32 && typeof data32 == "object" && !Array.isArray(data32)){
if(data32.kind === undefined){
data32.kind = "all";
}
if(data32.model === undefined){
data32.model = "all";
}
if(data32.search === undefined){
data32.search = "";
}
if(data32.selected === undefined){
data32.selected = null;
}
if(data32.status === undefined){
data32.status = "all";
}
const _errs87 = errors;
for(const key8 of Object.keys(data32)){
if(!(((((key8 === "kind") || (key8 === "model")) || (key8 === "search")) || (key8 === "selected")) || (key8 === "status"))){
validate24.errors = [{instancePath:instancePath+"/fom",schemaPath:"#/$defs/FomViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key8},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs87 === errors){
let data33 = data32.kind;
const _errs88 = errors;
if(errors === _errs88){
if(typeof data33 === "string"){
if(func63(data33) > 8192){
validate24.errors = [{instancePath:instancePath+"/fom/kind",schemaPath:"#/$defs/FomViewSettings/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/fom/kind",schemaPath:"#/$defs/FomViewSettings/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid17 = _errs88 === errors;
if(valid17){
let data34 = data32.model;
const _errs90 = errors;
if(errors === _errs90){
if(typeof data34 === "string"){
if(func63(data34) > 8192){
validate24.errors = [{instancePath:instancePath+"/fom/model",schemaPath:"#/$defs/FomViewSettings/properties/model/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/fom/model",schemaPath:"#/$defs/FomViewSettings/properties/model/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid17 = _errs90 === errors;
if(valid17){
let data35 = data32.search;
const _errs92 = errors;
if(errors === _errs92){
if(typeof data35 === "string"){
if(func63(data35) > 2048){
validate24.errors = [{instancePath:instancePath+"/fom/search",schemaPath:"#/$defs/FomViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/fom/search",schemaPath:"#/$defs/FomViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid17 = _errs92 === errors;
if(valid17){
let data36 = data32.selected;
const _errs94 = errors;
const _errs95 = errors;
let valid18 = false;
const _errs96 = errors;
if(errors === _errs96){
if(typeof data36 === "string"){
if(func63(data36) > 8192){
const err4 = {instancePath:instancePath+"/fom/selected",schemaPath:"#/$defs/FomViewSettings/properties/selected/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
if(vErrors === null){
vErrors = [err4];
}
else {
vErrors.push(err4);
}
errors++;
}
}
else {
const err5 = {instancePath:instancePath+"/fom/selected",schemaPath:"#/$defs/FomViewSettings/properties/selected/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err5];
}
else {
vErrors.push(err5);
}
errors++;
}
}
var _valid1 = _errs96 === errors;
valid18 = valid18 || _valid1;
const _errs98 = errors;
if(data36 !== null){
const err6 = {instancePath:instancePath+"/fom/selected",schemaPath:"#/$defs/FomViewSettings/properties/selected/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err6];
}
else {
vErrors.push(err6);
}
errors++;
}
var _valid1 = _errs98 === errors;
valid18 = valid18 || _valid1;
if(!valid18){
const err7 = {instancePath:instancePath+"/fom/selected",schemaPath:"#/$defs/FomViewSettings/properties/selected/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err7];
}
else {
vErrors.push(err7);
}
errors++;
validate24.errors = vErrors;
return false;
}
else {
errors = _errs95;
if(vErrors !== null){
if(_errs95){
vErrors.length = _errs95;
}
else {
vErrors = null;
}
}
}
var valid17 = _errs94 === errors;
if(valid17){
let data37 = data32.status;
const _errs100 = errors;
if(errors === _errs100){
if(typeof data37 === "string"){
if(func63(data37) > 8192){
validate24.errors = [{instancePath:instancePath+"/fom/status",schemaPath:"#/$defs/FomViewSettings/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/fom/status",schemaPath:"#/$defs/FomViewSettings/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid17 = _errs100 === errors;
}
}
}
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/fom",schemaPath:"#/$defs/FomViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs84 === errors;
if(valid0){
let data38 = data.interactions;
const _errs102 = errors;
const _errs103 = errors;
if(errors === _errs103){
if(data38 && typeof data38 == "object" && !Array.isArray(data38)){
if(data38.order === undefined){
data38.order = "all";
}
if(data38.search === undefined){
data38.search = "";
}
if(data38.status === undefined){
data38.status = "all";
}
const _errs105 = errors;
for(const key9 of Object.keys(data38)){
if(!(((key9 === "order") || (key9 === "search")) || (key9 === "status"))){
validate24.errors = [{instancePath:instancePath+"/interactions",schemaPath:"#/$defs/InteractionViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key9},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs105 === errors){
let data39 = data38.order;
const _errs106 = errors;
if(errors === _errs106){
if(typeof data39 === "string"){
if(func63(data39) > 8192){
validate24.errors = [{instancePath:instancePath+"/interactions/order",schemaPath:"#/$defs/InteractionViewSettings/properties/order/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/interactions/order",schemaPath:"#/$defs/InteractionViewSettings/properties/order/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid20 = _errs106 === errors;
if(valid20){
let data40 = data38.search;
const _errs108 = errors;
if(errors === _errs108){
if(typeof data40 === "string"){
if(func63(data40) > 2048){
validate24.errors = [{instancePath:instancePath+"/interactions/search",schemaPath:"#/$defs/InteractionViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/interactions/search",schemaPath:"#/$defs/InteractionViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid20 = _errs108 === errors;
if(valid20){
let data41 = data38.status;
const _errs110 = errors;
if(errors === _errs110){
if(typeof data41 === "string"){
if(func63(data41) > 8192){
validate24.errors = [{instancePath:instancePath+"/interactions/status",schemaPath:"#/$defs/InteractionViewSettings/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/interactions/status",schemaPath:"#/$defs/InteractionViewSettings/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid20 = _errs110 === errors;
}
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/interactions",schemaPath:"#/$defs/InteractionViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs102 === errors;
if(valid0){
let data42 = data.mom;
const _errs112 = errors;
const _errs113 = errors;
if(errors === _errs113){
if(data42 && typeof data42 == "object" && !Array.isArray(data42)){
if(data42.counter === undefined){
data42.counter = null;
}
if(data42.scope === undefined){
data42.scope = "all";
}
if(data42.search === undefined){
data42.search = "";
}
if(data42.service_status === undefined){
data42.service_status = "all";
}
const _errs115 = errors;
for(const key10 of Object.keys(data42)){
if(!((((key10 === "counter") || (key10 === "scope")) || (key10 === "search")) || (key10 === "service_status"))){
validate24.errors = [{instancePath:instancePath+"/mom",schemaPath:"#/$defs/MomViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key10},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs115 === errors){
let data43 = data42.counter;
const _errs116 = errors;
const _errs117 = errors;
let valid23 = false;
const _errs118 = errors;
if(errors === _errs118){
if(typeof data43 === "string"){
if(func63(data43) > 8192){
const err8 = {instancePath:instancePath+"/mom/counter",schemaPath:"#/$defs/MomViewSettings/properties/counter/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
if(vErrors === null){
vErrors = [err8];
}
else {
vErrors.push(err8);
}
errors++;
}
}
else {
const err9 = {instancePath:instancePath+"/mom/counter",schemaPath:"#/$defs/MomViewSettings/properties/counter/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err9];
}
else {
vErrors.push(err9);
}
errors++;
}
}
var _valid2 = _errs118 === errors;
valid23 = valid23 || _valid2;
const _errs120 = errors;
if(data43 !== null){
const err10 = {instancePath:instancePath+"/mom/counter",schemaPath:"#/$defs/MomViewSettings/properties/counter/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err10];
}
else {
vErrors.push(err10);
}
errors++;
}
var _valid2 = _errs120 === errors;
valid23 = valid23 || _valid2;
if(!valid23){
const err11 = {instancePath:instancePath+"/mom/counter",schemaPath:"#/$defs/MomViewSettings/properties/counter/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err11];
}
else {
vErrors.push(err11);
}
errors++;
validate24.errors = vErrors;
return false;
}
else {
errors = _errs117;
if(vErrors !== null){
if(_errs117){
vErrors.length = _errs117;
}
else {
vErrors = null;
}
}
}
var valid22 = _errs116 === errors;
if(valid22){
let data44 = data42.scope;
const _errs122 = errors;
if(errors === _errs122){
if(typeof data44 === "string"){
if(func63(data44) > 8192){
validate24.errors = [{instancePath:instancePath+"/mom/scope",schemaPath:"#/$defs/MomViewSettings/properties/scope/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/mom/scope",schemaPath:"#/$defs/MomViewSettings/properties/scope/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid22 = _errs122 === errors;
if(valid22){
let data45 = data42.search;
const _errs124 = errors;
if(errors === _errs124){
if(typeof data45 === "string"){
if(func63(data45) > 2048){
validate24.errors = [{instancePath:instancePath+"/mom/search",schemaPath:"#/$defs/MomViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/mom/search",schemaPath:"#/$defs/MomViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid22 = _errs124 === errors;
if(valid22){
let data46 = data42.service_status;
const _errs126 = errors;
if(errors === _errs126){
if(typeof data46 === "string"){
if(func63(data46) > 8192){
validate24.errors = [{instancePath:instancePath+"/mom/service_status",schemaPath:"#/$defs/MomViewSettings/properties/service_status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/mom/service_status",schemaPath:"#/$defs/MomViewSettings/properties/service_status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid22 = _errs126 === errors;
}
}
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/mom",schemaPath:"#/$defs/MomViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs112 === errors;
if(valid0){
let data47 = data.object_charts;
const _errs128 = errors;
if(errors === _errs128){
if(Array.isArray(data47)){
if(data47.length > 64){
validate24.errors = [{instancePath:instancePath+"/object_charts",schemaPath:"#/properties/object_charts/maxItems",keyword:"maxItems",params:{limit: 64},message:"must NOT have more than 64 items"}];
return false;
}
else {
var valid24 = true;
const len0 = data47.length;
for(let i0=0; i0<len0; i0++){
let data48 = data47[i0];
const _errs130 = errors;
const _errs131 = errors;
if(errors === _errs131){
if(data48 && typeof data48 == "object" && !Array.isArray(data48)){
if(data48.scope === undefined){
data48.scope = null;
}
let missing0;
if((((data48.object_key === undefined) || (!(func0.call(data48, "object_key")))) && (missing0 = "object_key")) || (((data48.attribute === undefined) || (!(func0.call(data48, "attribute")))) && (missing0 = "attribute"))){
validate24.errors = [{instancePath:instancePath+"/object_charts/" + i0,schemaPath:"#/$defs/ObjectChartChoice/required",keyword:"required",params:{missingProperty: missing0},message:"must have required property '"+missing0+"'"}];
return false;
}
else {
const _errs133 = errors;
for(const key11 of Object.keys(data48)){
if(!(((key11 === "attribute") || (key11 === "object_key")) || (key11 === "scope"))){
validate24.errors = [{instancePath:instancePath+"/object_charts/" + i0,schemaPath:"#/$defs/ObjectChartChoice/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key11},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs133 === errors){
if(data48.attribute !== undefined && func0.call(data48, "attribute")){
let data49 = data48.attribute;
const _errs134 = errors;
if(errors === _errs134){
if(typeof data49 === "string"){
if(func63(data49) > 8192){
validate24.errors = [{instancePath:instancePath+"/object_charts/" + i0+"/attribute",schemaPath:"#/$defs/ObjectChartChoice/properties/attribute/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
else {
if(func63(data49) < 1){
validate24.errors = [{instancePath:instancePath+"/object_charts/" + i0+"/attribute",schemaPath:"#/$defs/ObjectChartChoice/properties/attribute/minLength",keyword:"minLength",params:{limit: 1},message:"must NOT have fewer than 1 characters"}];
return false;
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/object_charts/" + i0+"/attribute",schemaPath:"#/$defs/ObjectChartChoice/properties/attribute/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid26 = _errs134 === errors;
}
else {
var valid26 = true;
}
if(valid26){
if(data48.object_key !== undefined && func0.call(data48, "object_key")){
let data50 = data48.object_key;
const _errs136 = errors;
if(errors === _errs136){
if(typeof data50 === "string"){
if(func63(data50) > 8192){
validate24.errors = [{instancePath:instancePath+"/object_charts/" + i0+"/object_key",schemaPath:"#/$defs/ObjectChartChoice/properties/object_key/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
else {
if(func63(data50) < 1){
validate24.errors = [{instancePath:instancePath+"/object_charts/" + i0+"/object_key",schemaPath:"#/$defs/ObjectChartChoice/properties/object_key/minLength",keyword:"minLength",params:{limit: 1},message:"must NOT have fewer than 1 characters"}];
return false;
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/object_charts/" + i0+"/object_key",schemaPath:"#/$defs/ObjectChartChoice/properties/object_key/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid26 = _errs136 === errors;
}
else {
var valid26 = true;
}
if(valid26){
let data51 = data48.scope;
const _errs138 = errors;
const _errs139 = errors;
let valid27 = false;
const _errs140 = errors;
if(errors === _errs140){
if(typeof data51 === "string"){
if(func63(data51) > 8192){
const err12 = {instancePath:instancePath+"/object_charts/" + i0+"/scope",schemaPath:"#/$defs/ObjectChartChoice/properties/scope/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
if(vErrors === null){
vErrors = [err12];
}
else {
vErrors.push(err12);
}
errors++;
}
}
else {
const err13 = {instancePath:instancePath+"/object_charts/" + i0+"/scope",schemaPath:"#/$defs/ObjectChartChoice/properties/scope/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err13];
}
else {
vErrors.push(err13);
}
errors++;
}
}
var _valid3 = _errs140 === errors;
valid27 = valid27 || _valid3;
const _errs142 = errors;
if(data51 !== null){
const err14 = {instancePath:instancePath+"/object_charts/" + i0+"/scope",schemaPath:"#/$defs/ObjectChartChoice/properties/scope/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err14];
}
else {
vErrors.push(err14);
}
errors++;
}
var _valid3 = _errs142 === errors;
valid27 = valid27 || _valid3;
if(!valid27){
const err15 = {instancePath:instancePath+"/object_charts/" + i0+"/scope",schemaPath:"#/$defs/ObjectChartChoice/properties/scope/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err15];
}
else {
vErrors.push(err15);
}
errors++;
validate24.errors = vErrors;
return false;
}
else {
errors = _errs139;
if(vErrors !== null){
if(_errs139){
vErrors.length = _errs139;
}
else {
vErrors = null;
}
}
}
var valid26 = _errs138 === errors;
}
}
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/object_charts/" + i0,schemaPath:"#/$defs/ObjectChartChoice/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid24 = _errs130 === errors;
if(!valid24){
break;
}
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/object_charts",schemaPath:"#/properties/object_charts/type",keyword:"type",params:{type: "array"},message:"must be array"}];
return false;
}
}
var valid0 = _errs128 === errors;
if(valid0){
let data52 = data.objects;
const _errs144 = errors;
const _errs145 = errors;
if(errors === _errs145){
if(data52 && typeof data52 == "object" && !Array.isArray(data52)){
if(data52.scope === undefined){
data52.scope = null;
}
if(data52.search === undefined){
data52.search = "";
}
if(data52.selected === undefined){
data52.selected = "";
}
if(data52.status === undefined){
data52.status = "all";
}
const _errs147 = errors;
for(const key12 of Object.keys(data52)){
if(!((((key12 === "scope") || (key12 === "search")) || (key12 === "selected")) || (key12 === "status"))){
validate24.errors = [{instancePath:instancePath+"/objects",schemaPath:"#/$defs/ObjectViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key12},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs147 === errors){
let data53 = data52.scope;
const _errs148 = errors;
const _errs149 = errors;
let valid30 = false;
const _errs150 = errors;
if(errors === _errs150){
if(typeof data53 === "string"){
if(func63(data53) > 8192){
const err16 = {instancePath:instancePath+"/objects/scope",schemaPath:"#/$defs/ObjectViewSettings/properties/scope/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
if(vErrors === null){
vErrors = [err16];
}
else {
vErrors.push(err16);
}
errors++;
}
}
else {
const err17 = {instancePath:instancePath+"/objects/scope",schemaPath:"#/$defs/ObjectViewSettings/properties/scope/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err17];
}
else {
vErrors.push(err17);
}
errors++;
}
}
var _valid4 = _errs150 === errors;
valid30 = valid30 || _valid4;
const _errs152 = errors;
if(data53 !== null){
const err18 = {instancePath:instancePath+"/objects/scope",schemaPath:"#/$defs/ObjectViewSettings/properties/scope/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err18];
}
else {
vErrors.push(err18);
}
errors++;
}
var _valid4 = _errs152 === errors;
valid30 = valid30 || _valid4;
if(!valid30){
const err19 = {instancePath:instancePath+"/objects/scope",schemaPath:"#/$defs/ObjectViewSettings/properties/scope/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err19];
}
else {
vErrors.push(err19);
}
errors++;
validate24.errors = vErrors;
return false;
}
else {
errors = _errs149;
if(vErrors !== null){
if(_errs149){
vErrors.length = _errs149;
}
else {
vErrors = null;
}
}
}
var valid29 = _errs148 === errors;
if(valid29){
let data54 = data52.search;
const _errs154 = errors;
if(errors === _errs154){
if(typeof data54 === "string"){
if(func63(data54) > 2048){
validate24.errors = [{instancePath:instancePath+"/objects/search",schemaPath:"#/$defs/ObjectViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/objects/search",schemaPath:"#/$defs/ObjectViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid29 = _errs154 === errors;
if(valid29){
let data55 = data52.selected;
const _errs156 = errors;
if(errors === _errs156){
if(typeof data55 === "string"){
if(func63(data55) > 8192){
validate24.errors = [{instancePath:instancePath+"/objects/selected",schemaPath:"#/$defs/ObjectViewSettings/properties/selected/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/objects/selected",schemaPath:"#/$defs/ObjectViewSettings/properties/selected/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid29 = _errs156 === errors;
if(valid29){
let data56 = data52.status;
const _errs158 = errors;
if(errors === _errs158){
if(typeof data56 === "string"){
if(func63(data56) > 8192){
validate24.errors = [{instancePath:instancePath+"/objects/status",schemaPath:"#/$defs/ObjectViewSettings/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/objects/status",schemaPath:"#/$defs/ObjectViewSettings/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid29 = _errs158 === errors;
}
}
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/objects",schemaPath:"#/$defs/ObjectViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs144 === errors;
if(valid0){
let data57 = data.ownership;
const _errs160 = errors;
const _errs161 = errors;
if(errors === _errs161){
if(data57 && typeof data57 == "object" && !Array.isArray(data57)){
if(data57.owner_state === undefined){
data57.owner_state = "all";
}
if(data57.request_status === undefined){
data57.request_status = "all";
}
if(data57.scope === undefined){
data57.scope = "all";
}
if(data57.search === undefined){
data57.search = "";
}
const _errs163 = errors;
for(const key13 of Object.keys(data57)){
if(!((((key13 === "owner_state") || (key13 === "request_status")) || (key13 === "scope")) || (key13 === "search"))){
validate24.errors = [{instancePath:instancePath+"/ownership",schemaPath:"#/$defs/OwnershipViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key13},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs163 === errors){
let data58 = data57.owner_state;
const _errs164 = errors;
if(errors === _errs164){
if(typeof data58 === "string"){
if(func63(data58) > 8192){
validate24.errors = [{instancePath:instancePath+"/ownership/owner_state",schemaPath:"#/$defs/OwnershipViewSettings/properties/owner_state/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/ownership/owner_state",schemaPath:"#/$defs/OwnershipViewSettings/properties/owner_state/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid32 = _errs164 === errors;
if(valid32){
let data59 = data57.request_status;
const _errs166 = errors;
if(errors === _errs166){
if(typeof data59 === "string"){
if(func63(data59) > 8192){
validate24.errors = [{instancePath:instancePath+"/ownership/request_status",schemaPath:"#/$defs/OwnershipViewSettings/properties/request_status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/ownership/request_status",schemaPath:"#/$defs/OwnershipViewSettings/properties/request_status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid32 = _errs166 === errors;
if(valid32){
let data60 = data57.scope;
const _errs168 = errors;
if(errors === _errs168){
if(typeof data60 === "string"){
if(func63(data60) > 8192){
validate24.errors = [{instancePath:instancePath+"/ownership/scope",schemaPath:"#/$defs/OwnershipViewSettings/properties/scope/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/ownership/scope",schemaPath:"#/$defs/OwnershipViewSettings/properties/scope/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid32 = _errs168 === errors;
if(valid32){
let data61 = data57.search;
const _errs170 = errors;
if(errors === _errs170){
if(typeof data61 === "string"){
if(func63(data61) > 2048){
validate24.errors = [{instancePath:instancePath+"/ownership/search",schemaPath:"#/$defs/OwnershipViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/ownership/search",schemaPath:"#/$defs/OwnershipViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid32 = _errs170 === errors;
}
}
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/ownership",schemaPath:"#/$defs/OwnershipViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs160 === errors;
if(valid0){
let data62 = data.persistence;
const _errs172 = errors;
const _errs173 = errors;
if(errors === _errs173){
if(data62 && typeof data62 == "object" && !Array.isArray(data62)){
if(data62.kind === undefined){
data62.kind = "all";
}
if(data62.phase === undefined){
data62.phase = "all";
}
if(data62.scope === undefined){
data62.scope = "all";
}
if(data62.search === undefined){
data62.search = "";
}
if(data62.selected === undefined){
data62.selected = "all";
}
const _errs175 = errors;
for(const key14 of Object.keys(data62)){
if(!(((((key14 === "kind") || (key14 === "phase")) || (key14 === "scope")) || (key14 === "search")) || (key14 === "selected"))){
validate24.errors = [{instancePath:instancePath+"/persistence",schemaPath:"#/$defs/CoordinationViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key14},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs175 === errors){
let data63 = data62.kind;
const _errs176 = errors;
if(errors === _errs176){
if(typeof data63 === "string"){
if(func63(data63) > 8192){
validate24.errors = [{instancePath:instancePath+"/persistence/kind",schemaPath:"#/$defs/CoordinationViewSettings/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/persistence/kind",schemaPath:"#/$defs/CoordinationViewSettings/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid34 = _errs176 === errors;
if(valid34){
let data64 = data62.phase;
const _errs178 = errors;
if(errors === _errs178){
if(typeof data64 === "string"){
if(func63(data64) > 8192){
validate24.errors = [{instancePath:instancePath+"/persistence/phase",schemaPath:"#/$defs/CoordinationViewSettings/properties/phase/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/persistence/phase",schemaPath:"#/$defs/CoordinationViewSettings/properties/phase/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid34 = _errs178 === errors;
if(valid34){
let data65 = data62.scope;
const _errs180 = errors;
if(errors === _errs180){
if(typeof data65 === "string"){
if(func63(data65) > 8192){
validate24.errors = [{instancePath:instancePath+"/persistence/scope",schemaPath:"#/$defs/CoordinationViewSettings/properties/scope/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/persistence/scope",schemaPath:"#/$defs/CoordinationViewSettings/properties/scope/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid34 = _errs180 === errors;
if(valid34){
let data66 = data62.search;
const _errs182 = errors;
if(errors === _errs182){
if(typeof data66 === "string"){
if(func63(data66) > 2048){
validate24.errors = [{instancePath:instancePath+"/persistence/search",schemaPath:"#/$defs/CoordinationViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/persistence/search",schemaPath:"#/$defs/CoordinationViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid34 = _errs182 === errors;
if(valid34){
let data67 = data62.selected;
const _errs184 = errors;
if(errors === _errs184){
if(typeof data67 === "string"){
if(func63(data67) > 8192){
validate24.errors = [{instancePath:instancePath+"/persistence/selected",schemaPath:"#/$defs/CoordinationViewSettings/properties/selected/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/persistence/selected",schemaPath:"#/$defs/CoordinationViewSettings/properties/selected/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid34 = _errs184 === errors;
}
}
}
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/persistence",schemaPath:"#/$defs/CoordinationViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs172 === errors;
if(valid0){
let data68 = data.regions;
const _errs186 = errors;
const _errs187 = errors;
if(errors === _errs187){
if(data68 && typeof data68 == "object" && !Array.isArray(data68)){
if(data68.evidence === undefined){
data68.evidence = "all";
}
if(data68.kind === undefined){
data68.kind = "all";
}
if(data68.search === undefined){
data68.search = "";
}
if(data68.selected === undefined){
data68.selected = null;
}
if(data68.status === undefined){
data68.status = "all";
}
const _errs189 = errors;
for(const key15 of Object.keys(data68)){
if(!(((((key15 === "evidence") || (key15 === "kind")) || (key15 === "search")) || (key15 === "selected")) || (key15 === "status"))){
validate24.errors = [{instancePath:instancePath+"/regions",schemaPath:"#/$defs/RegionViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key15},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs189 === errors){
let data69 = data68.evidence;
const _errs190 = errors;
if(errors === _errs190){
if(typeof data69 === "string"){
if(func63(data69) > 8192){
validate24.errors = [{instancePath:instancePath+"/regions/evidence",schemaPath:"#/$defs/RegionViewSettings/properties/evidence/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/regions/evidence",schemaPath:"#/$defs/RegionViewSettings/properties/evidence/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid36 = _errs190 === errors;
if(valid36){
let data70 = data68.kind;
const _errs192 = errors;
if(errors === _errs192){
if(typeof data70 === "string"){
if(func63(data70) > 8192){
validate24.errors = [{instancePath:instancePath+"/regions/kind",schemaPath:"#/$defs/RegionViewSettings/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/regions/kind",schemaPath:"#/$defs/RegionViewSettings/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid36 = _errs192 === errors;
if(valid36){
let data71 = data68.search;
const _errs194 = errors;
if(errors === _errs194){
if(typeof data71 === "string"){
if(func63(data71) > 2048){
validate24.errors = [{instancePath:instancePath+"/regions/search",schemaPath:"#/$defs/RegionViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/regions/search",schemaPath:"#/$defs/RegionViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid36 = _errs194 === errors;
if(valid36){
let data72 = data68.selected;
const _errs196 = errors;
const _errs197 = errors;
let valid37 = false;
const _errs198 = errors;
if(errors === _errs198){
if(typeof data72 === "string"){
if(func63(data72) > 8192){
const err20 = {instancePath:instancePath+"/regions/selected",schemaPath:"#/$defs/RegionViewSettings/properties/selected/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
if(vErrors === null){
vErrors = [err20];
}
else {
vErrors.push(err20);
}
errors++;
}
}
else {
const err21 = {instancePath:instancePath+"/regions/selected",schemaPath:"#/$defs/RegionViewSettings/properties/selected/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err21];
}
else {
vErrors.push(err21);
}
errors++;
}
}
var _valid5 = _errs198 === errors;
valid37 = valid37 || _valid5;
const _errs200 = errors;
if(data72 !== null){
const err22 = {instancePath:instancePath+"/regions/selected",schemaPath:"#/$defs/RegionViewSettings/properties/selected/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err22];
}
else {
vErrors.push(err22);
}
errors++;
}
var _valid5 = _errs200 === errors;
valid37 = valid37 || _valid5;
if(!valid37){
const err23 = {instancePath:instancePath+"/regions/selected",schemaPath:"#/$defs/RegionViewSettings/properties/selected/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err23];
}
else {
vErrors.push(err23);
}
errors++;
validate24.errors = vErrors;
return false;
}
else {
errors = _errs197;
if(vErrors !== null){
if(_errs197){
vErrors.length = _errs197;
}
else {
vErrors = null;
}
}
}
var valid36 = _errs196 === errors;
if(valid36){
let data73 = data68.status;
const _errs202 = errors;
if(errors === _errs202){
if(typeof data73 === "string"){
if(func63(data73) > 8192){
validate24.errors = [{instancePath:instancePath+"/regions/status",schemaPath:"#/$defs/RegionViewSettings/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/regions/status",schemaPath:"#/$defs/RegionViewSettings/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid36 = _errs202 === errors;
}
}
}
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/regions",schemaPath:"#/$defs/RegionViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs186 === errors;
if(valid0){
let data74 = data.selected_federate;
const _errs204 = errors;
if(errors === _errs204){
if(typeof data74 === "string"){
if(func63(data74) > 8192){
validate24.errors = [{instancePath:instancePath+"/selected_federate",schemaPath:"#/properties/selected_federate/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/selected_federate",schemaPath:"#/properties/selected_federate/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs204 === errors;
if(valid0){
let data75 = data.synchronization;
const _errs206 = errors;
const _errs207 = errors;
if(errors === _errs207){
if(data75 && typeof data75 == "object" && !Array.isArray(data75)){
if(data75.kind === undefined){
data75.kind = "all";
}
if(data75.phase === undefined){
data75.phase = "all";
}
if(data75.scope === undefined){
data75.scope = "all";
}
if(data75.search === undefined){
data75.search = "";
}
if(data75.selected === undefined){
data75.selected = "all";
}
const _errs209 = errors;
for(const key16 of Object.keys(data75)){
if(!(((((key16 === "kind") || (key16 === "phase")) || (key16 === "scope")) || (key16 === "search")) || (key16 === "selected"))){
validate24.errors = [{instancePath:instancePath+"/synchronization",schemaPath:"#/$defs/CoordinationViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key16},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs209 === errors){
let data76 = data75.kind;
const _errs210 = errors;
if(errors === _errs210){
if(typeof data76 === "string"){
if(func63(data76) > 8192){
validate24.errors = [{instancePath:instancePath+"/synchronization/kind",schemaPath:"#/$defs/CoordinationViewSettings/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/synchronization/kind",schemaPath:"#/$defs/CoordinationViewSettings/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid39 = _errs210 === errors;
if(valid39){
let data77 = data75.phase;
const _errs212 = errors;
if(errors === _errs212){
if(typeof data77 === "string"){
if(func63(data77) > 8192){
validate24.errors = [{instancePath:instancePath+"/synchronization/phase",schemaPath:"#/$defs/CoordinationViewSettings/properties/phase/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/synchronization/phase",schemaPath:"#/$defs/CoordinationViewSettings/properties/phase/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid39 = _errs212 === errors;
if(valid39){
let data78 = data75.scope;
const _errs214 = errors;
if(errors === _errs214){
if(typeof data78 === "string"){
if(func63(data78) > 8192){
validate24.errors = [{instancePath:instancePath+"/synchronization/scope",schemaPath:"#/$defs/CoordinationViewSettings/properties/scope/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/synchronization/scope",schemaPath:"#/$defs/CoordinationViewSettings/properties/scope/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid39 = _errs214 === errors;
if(valid39){
let data79 = data75.search;
const _errs216 = errors;
if(errors === _errs216){
if(typeof data79 === "string"){
if(func63(data79) > 2048){
validate24.errors = [{instancePath:instancePath+"/synchronization/search",schemaPath:"#/$defs/CoordinationViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/synchronization/search",schemaPath:"#/$defs/CoordinationViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid39 = _errs216 === errors;
if(valid39){
let data80 = data75.selected;
const _errs218 = errors;
if(errors === _errs218){
if(typeof data80 === "string"){
if(func63(data80) > 8192){
validate24.errors = [{instancePath:instancePath+"/synchronization/selected",schemaPath:"#/$defs/CoordinationViewSettings/properties/selected/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/synchronization/selected",schemaPath:"#/$defs/CoordinationViewSettings/properties/selected/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid39 = _errs218 === errors;
}
}
}
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/synchronization",schemaPath:"#/$defs/CoordinationViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs206 === errors;
if(valid0){
let data81 = data.time;
const _errs220 = errors;
const _errs221 = errors;
if(errors === _errs221){
if(data81 && typeof data81 == "object" && !Array.isArray(data81)){
if(data81.grants_only === undefined){
data81.grants_only = false;
}
if(data81.selected === undefined){
data81.selected = null;
}
const _errs223 = errors;
for(const key17 of Object.keys(data81)){
if(!((key17 === "grants_only") || (key17 === "selected"))){
validate24.errors = [{instancePath:instancePath+"/time",schemaPath:"#/$defs/TimeViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key17},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs223 === errors){
const _errs224 = errors;
if(typeof data81.grants_only !== "boolean"){
validate24.errors = [{instancePath:instancePath+"/time/grants_only",schemaPath:"#/$defs/TimeViewSettings/properties/grants_only/type",keyword:"type",params:{type: "boolean"},message:"must be boolean"}];
return false;
}
var valid41 = _errs224 === errors;
if(valid41){
let data83 = data81.selected;
const _errs226 = errors;
const _errs227 = errors;
let valid42 = false;
const _errs228 = errors;
if(errors === _errs228){
if(typeof data83 === "string"){
if(func63(data83) > 8192){
const err24 = {instancePath:instancePath+"/time/selected",schemaPath:"#/$defs/TimeViewSettings/properties/selected/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
if(vErrors === null){
vErrors = [err24];
}
else {
vErrors.push(err24);
}
errors++;
}
}
else {
const err25 = {instancePath:instancePath+"/time/selected",schemaPath:"#/$defs/TimeViewSettings/properties/selected/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err25];
}
else {
vErrors.push(err25);
}
errors++;
}
}
var _valid6 = _errs228 === errors;
valid42 = valid42 || _valid6;
const _errs230 = errors;
if(data83 !== null){
const err26 = {instancePath:instancePath+"/time/selected",schemaPath:"#/$defs/TimeViewSettings/properties/selected/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err26];
}
else {
vErrors.push(err26);
}
errors++;
}
var _valid6 = _errs230 === errors;
valid42 = valid42 || _valid6;
if(!valid42){
const err27 = {instancePath:instancePath+"/time/selected",schemaPath:"#/$defs/TimeViewSettings/properties/selected/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err27];
}
else {
vErrors.push(err27);
}
errors++;
validate24.errors = vErrors;
return false;
}
else {
errors = _errs227;
if(vErrors !== null){
if(_errs227){
vErrors.length = _errs227;
}
else {
vErrors = null;
}
}
}
var valid41 = _errs226 === errors;
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/time",schemaPath:"#/$defs/TimeViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs220 === errors;
if(valid0){
let data84 = data.topology;
const _errs232 = errors;
const _errs233 = errors;
if(errors === _errs233){
if(data84 && typeof data84 == "object" && !Array.isArray(data84)){
if(data84.evidence === undefined){
data84.evidence = "all";
}
if(data84.focused === undefined){
data84.focused = "all";
}
if(data84.kind === undefined){
data84.kind = "all";
}
if(data84.search === undefined){
data84.search = "";
}
if(data84.source === undefined){
data84.source = "all";
}
if(data84.status === undefined){
data84.status = "all";
}
const _errs235 = errors;
for(const key18 of Object.keys(data84)){
if(!((((((key18 === "evidence") || (key18 === "focused")) || (key18 === "kind")) || (key18 === "search")) || (key18 === "source")) || (key18 === "status"))){
validate24.errors = [{instancePath:instancePath+"/topology",schemaPath:"#/$defs/TopologyViewSettings/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key18},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs235 === errors){
let data85 = data84.evidence;
const _errs236 = errors;
if(errors === _errs236){
if(typeof data85 === "string"){
if(func63(data85) > 8192){
validate24.errors = [{instancePath:instancePath+"/topology/evidence",schemaPath:"#/$defs/TopologyViewSettings/properties/evidence/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/topology/evidence",schemaPath:"#/$defs/TopologyViewSettings/properties/evidence/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid44 = _errs236 === errors;
if(valid44){
let data86 = data84.focused;
const _errs238 = errors;
if(errors === _errs238){
if(typeof data86 === "string"){
if(func63(data86) > 8192){
validate24.errors = [{instancePath:instancePath+"/topology/focused",schemaPath:"#/$defs/TopologyViewSettings/properties/focused/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/topology/focused",schemaPath:"#/$defs/TopologyViewSettings/properties/focused/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid44 = _errs238 === errors;
if(valid44){
let data87 = data84.kind;
const _errs240 = errors;
if(errors === _errs240){
if(typeof data87 === "string"){
if(func63(data87) > 8192){
validate24.errors = [{instancePath:instancePath+"/topology/kind",schemaPath:"#/$defs/TopologyViewSettings/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/topology/kind",schemaPath:"#/$defs/TopologyViewSettings/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid44 = _errs240 === errors;
if(valid44){
let data88 = data84.search;
const _errs242 = errors;
if(errors === _errs242){
if(typeof data88 === "string"){
if(func63(data88) > 2048){
validate24.errors = [{instancePath:instancePath+"/topology/search",schemaPath:"#/$defs/TopologyViewSettings/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/topology/search",schemaPath:"#/$defs/TopologyViewSettings/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid44 = _errs242 === errors;
if(valid44){
let data89 = data84.source;
const _errs244 = errors;
if(errors === _errs244){
if(typeof data89 === "string"){
if(func63(data89) > 8192){
validate24.errors = [{instancePath:instancePath+"/topology/source",schemaPath:"#/$defs/TopologyViewSettings/properties/source/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/topology/source",schemaPath:"#/$defs/TopologyViewSettings/properties/source/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid44 = _errs244 === errors;
if(valid44){
let data90 = data84.status;
const _errs246 = errors;
if(errors === _errs246){
if(typeof data90 === "string"){
if(func63(data90) > 8192){
validate24.errors = [{instancePath:instancePath+"/topology/status",schemaPath:"#/$defs/TopologyViewSettings/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate24.errors = [{instancePath:instancePath+"/topology/status",schemaPath:"#/$defs/TopologyViewSettings/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid44 = _errs246 === errors;
}
}
}
}
}
}
}
else {
validate24.errors = [{instancePath:instancePath+"/topology",schemaPath:"#/$defs/TopologyViewSettings/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
var valid0 = _errs232 === errors;
if(valid0){
let data91 = data.view;
const _errs248 = errors;
if(typeof data91 !== "string"){
validate24.errors = [{instancePath:instancePath+"/view",schemaPath:"#/properties/view/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
if(!(((((((((((((((((((data91 === "federation") || (data91 === "federates")) || (data91 === "objects")) || (data91 === "declarations")) || (data91 === "topology")) || (data91 === "interactions")) || (data91 === "ownership")) || (data91 === "regions")) || (data91 === "mom")) || (data91 === "fom")) || (data91 === "time")) || (data91 === "synchronization")) || (data91 === "persistence")) || (data91 === "events")) || (data91 === "inspectors")) || (data91 === "recordings")) || (data91 === "alerts")) || (data91 === "actions")) || (data91 === "diagnostics"))){
validate24.errors = [{instancePath:instancePath+"/view",schemaPath:"#/properties/view/enum",keyword:"enum",params:{allowedValues: schema52.properties.view.enum},message:"must be equal to one of the allowed values"}];
return false;
}
var valid0 = _errs248 === errors;
if(valid0){
let data92 = data.window_seconds;
const _errs250 = errors;
if(!(((typeof data92 == "number") && (!(data92 % 1) && !isNaN(data92))) && (isFinite(data92)))){
validate24.errors = [{instancePath:instancePath+"/window_seconds",schemaPath:"#/properties/window_seconds/type",keyword:"type",params:{type: "integer"},message:"must be integer"}];
return false;
}
if(!((((data92 === 30) || (data92 === 300)) || (data92 === 900)) || (data92 === 3600))){
validate24.errors = [{instancePath:instancePath+"/window_seconds",schemaPath:"#/properties/window_seconds/enum",keyword:"enum",params:{allowedValues: schema52.properties.window_seconds.enum},message:"must be equal to one of the allowed values"}];
return false;
}
var valid0 = _errs250 === errors;
}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
}
else {
validate24.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate24.errors = vErrors;
return errors === 0;
}
validate24.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema71 = {"additionalProperties":false,"properties":{"name":{"maxLength":128,"minLength":1,"pattern":"\\S","title":"Name","type":"string"},"settings":{"$ref":"#/$defs/WorkspaceSettings","default":{"alerts":{"search":"","status":"all"},"declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"events":{"family":"","federate":"","search":"","source":"","type":""},"federate_declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"federate_events":{"family":"","federate":"","search":"","source":"","type":""},"federate_time":{"grants_only":false,"selected":null},"federation":{"distribution":"event_types"},"fom":{"kind":"all","model":"all","search":"","selected":null,"status":"all"},"interactions":{"order":"all","search":"","status":"all"},"mom":{"counter":null,"scope":"all","search":"","service_status":"all"},"object_charts":[],"objects":{"scope":null,"search":"","selected":"","status":"all"},"ownership":{"owner_state":"all","request_status":"all","scope":"all","search":""},"persistence":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"regions":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"},"selected_federate":"","synchronization":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"time":{"grants_only":false,"selected":null},"topology":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"},"view":"federation","window_seconds":300}},"workspace_version":{"const":"umbra-observer-workspace-v1","title":"Workspace Version","type":"string"}},"required":["workspace_version","name"],"title":"WorkspaceDocument","type":"object"};

function validate26(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate26.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.settings === undefined){
data.settings = {"alerts":{"search":"","status":"all"},"declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"events":{"family":"","federate":"","search":"","source":"","type":""},"federate_declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"federate_events":{"family":"","federate":"","search":"","source":"","type":""},"federate_time":{"grants_only":false,"selected":null},"federation":{"distribution":"event_types"},"fom":{"kind":"all","model":"all","search":"","selected":null,"status":"all"},"interactions":{"order":"all","search":"","status":"all"},"mom":{"counter":null,"scope":"all","search":"","service_status":"all"},"object_charts":[],"objects":{"scope":null,"search":"","selected":"","status":"all"},"ownership":{"owner_state":"all","request_status":"all","scope":"all","search":""},"persistence":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"regions":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"},"selected_federate":"","synchronization":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"time":{"grants_only":false,"selected":null},"topology":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"},"view":"federation","window_seconds":300};
}
let missing0;
if((((data.workspace_version === undefined) || (!(func0.call(data, "workspace_version")))) && (missing0 = "workspace_version")) || (((data.name === undefined) || (!(func0.call(data, "name")))) && (missing0 = "name"))){
validate26.errors = [{instancePath,schemaPath:"#/required",keyword:"required",params:{missingProperty: missing0},message:"must have required property '"+missing0+"'"}];
return false;
}
else {
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((key0 === "name") || (key0 === "settings")) || (key0 === "workspace_version"))){
validate26.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
if(data.name !== undefined && func0.call(data, "name")){
let data0 = data.name;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 128){
validate26.errors = [{instancePath:instancePath+"/name",schemaPath:"#/properties/name/maxLength",keyword:"maxLength",params:{limit: 128},message:"must NOT have more than 128 characters"}];
return false;
}
else {
if(func63(data0) < 1){
validate26.errors = [{instancePath:instancePath+"/name",schemaPath:"#/properties/name/minLength",keyword:"minLength",params:{limit: 1},message:"must NOT have fewer than 1 characters"}];
return false;
}
else {
if(!pattern4.test(data0)){
validate26.errors = [{instancePath:instancePath+"/name",schemaPath:"#/properties/name/pattern",keyword:"pattern",params:{pattern: "\\S"},message:"must match pattern \""+"\\S"+"\""}];
return false;
}
}
}
}
else {
validate26.errors = [{instancePath:instancePath+"/name",schemaPath:"#/properties/name/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
}
else {
var valid0 = true;
}
if(valid0){
const _errs4 = errors;
if(!(validate24(data.settings, {instancePath:instancePath+"/settings",parentData:data,parentDataProperty:"settings",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate24.errors : vErrors.concat(validate24.errors);
errors = vErrors.length;
}
var valid0 = _errs4 === errors;
if(valid0){
if(data.workspace_version !== undefined && func0.call(data, "workspace_version")){
let data2 = data.workspace_version;
const _errs5 = errors;
if(typeof data2 !== "string"){
validate26.errors = [{instancePath:instancePath+"/workspace_version",schemaPath:"#/properties/workspace_version/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
if("umbra-observer-workspace-v1" !== data2){
validate26.errors = [{instancePath:instancePath+"/workspace_version",schemaPath:"#/properties/workspace_version/const",keyword:"const",params:{allowedValue: "umbra-observer-workspace-v1"},message:"must be equal to constant"}];
return false;
}
var valid0 = _errs5 === errors;
}
else {
var valid0 = true;
}
}
}
}
}
}
else {
validate26.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate26.errors = vErrors;
return errors === 0;
}
validate26.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};


function validate23(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
/*# sourceURL="urn:umbra:observer:workspace-library:v1" */;
let vErrors = null;
let errors = 0;
const evaluated0 = validate23.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.current === undefined){
data.current = {"alerts":{"search":"","status":"all"},"declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"events":{"family":"","federate":"","search":"","source":"","type":""},"federate_declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"federate_events":{"family":"","federate":"","search":"","source":"","type":""},"federate_time":{"grants_only":false,"selected":null},"federation":{"distribution":"event_types"},"fom":{"kind":"all","model":"all","search":"","selected":null,"status":"all"},"interactions":{"order":"all","search":"","status":"all"},"mom":{"counter":null,"scope":"all","search":"","service_status":"all"},"object_charts":[],"objects":{"scope":null,"search":"","selected":"","status":"all"},"ownership":{"owner_state":"all","request_status":"all","scope":"all","search":""},"persistence":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"regions":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"},"selected_federate":"","synchronization":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"time":{"grants_only":false,"selected":null},"topology":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"},"view":"federation","window_seconds":300};
}
if(data.saved === undefined){
data.saved = [];
}
let missing0;
if(((data.library_version === undefined) || (!(func0.call(data, "library_version")))) && (missing0 = "library_version")){
validate23.errors = [{instancePath,schemaPath:"#/required",keyword:"required",params:{missingProperty: missing0},message:"must have required property '"+missing0+"'"}];
return false;
}
else {
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((key0 === "current") || (key0 === "library_version")) || (key0 === "saved"))){
validate23.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
const _errs2 = errors;
if(!(validate24(data.current, {instancePath:instancePath+"/current",parentData:data,parentDataProperty:"current",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate24.errors : vErrors.concat(validate24.errors);
errors = vErrors.length;
}
var valid0 = _errs2 === errors;
if(valid0){
if(data.library_version !== undefined && func0.call(data, "library_version")){
let data1 = data.library_version;
const _errs3 = errors;
if(typeof data1 !== "string"){
validate23.errors = [{instancePath:instancePath+"/library_version",schemaPath:"#/properties/library_version/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
if("umbra-observer-workspace-library-v1" !== data1){
validate23.errors = [{instancePath:instancePath+"/library_version",schemaPath:"#/properties/library_version/const",keyword:"const",params:{allowedValue: "umbra-observer-workspace-library-v1"},message:"must be equal to constant"}];
return false;
}
var valid0 = _errs3 === errors;
}
else {
var valid0 = true;
}
if(valid0){
let data2 = data.saved;
const _errs5 = errors;
if(errors === _errs5){
if(Array.isArray(data2)){
if(data2.length > 12){
validate23.errors = [{instancePath:instancePath+"/saved",schemaPath:"#/properties/saved/maxItems",keyword:"maxItems",params:{limit: 12},message:"must NOT have more than 12 items"}];
return false;
}
else {
var valid1 = true;
const len0 = data2.length;
for(let i0=0; i0<len0; i0++){
const _errs7 = errors;
if(!(validate26(data2[i0], {instancePath:instancePath+"/saved/" + i0,parentData:data2,parentDataProperty:i0,rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate26.errors : vErrors.concat(validate26.errors);
errors = vErrors.length;
}
var valid1 = _errs7 === errors;
if(!valid1){
break;
}
}
}
}
else {
validate23.errors = [{instancePath:instancePath+"/saved",schemaPath:"#/properties/saved/type",keyword:"type",params:{type: "array"},message:"must be array"}];
return false;
}
}
var valid0 = _errs5 === errors;
}
}
}
}
}
else {
validate23.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate23.errors = vErrors;
return errors === 0;
}
validate23.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};
