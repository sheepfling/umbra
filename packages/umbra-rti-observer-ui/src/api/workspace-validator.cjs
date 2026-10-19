// Generated from the named Python workspace models. Do not edit.
// Contract SHA-256: a6ee24bb0249988a4941275e4ca636a04ed062c2a3840e08f1ef414c1b9bd956
"use strict";
exports.validateWorkspace = validate52;
const schema19 = {"$defs":{"AlertViewSettings":{"additionalProperties":false,"properties":{"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"AlertViewSettings","type":"object"},"CoordinationViewSettings":{"additionalProperties":false,"properties":{"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"phase":{"default":"all","maxLength":8192,"title":"Phase","type":"string"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"default":"all","maxLength":8192,"title":"Selected","type":"string"}},"title":"CoordinationViewSettings","type":"object"},"DeclarationViewSettings":{"additionalProperties":false,"properties":{"direction":{"default":"all","maxLength":8192,"title":"Direction","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"perspective":{"default":"all","maxLength":8192,"title":"Perspective","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"DeclarationViewSettings","type":"object"},"EventViewSettings":{"additionalProperties":false,"properties":{"family":{"default":"","maxLength":8192,"title":"Family","type":"string"},"federate":{"default":"","maxLength":8192,"title":"Federate","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"source":{"default":"","maxLength":8192,"title":"Source","type":"string"},"type":{"default":"","maxLength":8192,"title":"Type","type":"string"}},"title":"EventViewSettings","type":"object"},"FederateListSettings":{"additionalProperties":false,"properties":{"basis":{"default":"all","enum":["all","explicit-join","legacy-provenance","legacy-name"],"title":"Basis","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"}},"title":"FederateListSettings","type":"object"},"FederateWorkspaceSettings":{"additionalProperties":false,"properties":{"declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"participant":{"maxLength":8192,"minLength":1,"title":"Participant","type":"string"},"time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}}},"required":["participant"],"title":"FederateWorkspaceSettings","type":"object"},"FederationViewSettings":{"additionalProperties":false,"properties":{"distribution":{"default":"event_types","enum":["event_types","sources","families"],"title":"Distribution","type":"string"}},"title":"FederationViewSettings","type":"object"},"FomViewSettings":{"additionalProperties":false,"properties":{"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"model":{"default":"all","maxLength":8192,"title":"Model","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"FomViewSettings","type":"object"},"InteractionViewSettings":{"additionalProperties":false,"properties":{"order":{"default":"all","maxLength":8192,"title":"Order","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"InteractionViewSettings","type":"object"},"MomViewSettings":{"additionalProperties":false,"properties":{"counter":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Counter"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"service_status":{"default":"all","maxLength":8192,"title":"Service Status","type":"string"}},"title":"MomViewSettings","type":"object"},"ObjectChartChoice":{"additionalProperties":false,"properties":{"attribute":{"maxLength":8192,"minLength":1,"title":"Attribute","type":"string"},"object_key":{"maxLength":8192,"minLength":1,"title":"Object Key","type":"string"},"scope":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Scope"}},"required":["object_key","attribute"],"title":"ObjectChartChoice","type":"object"},"ObjectViewSettings":{"additionalProperties":false,"properties":{"scope":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Scope"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"default":"","maxLength":8192,"title":"Selected","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"ObjectViewSettings","type":"object"},"OwnershipViewSettings":{"additionalProperties":false,"properties":{"owner_state":{"default":"all","maxLength":8192,"title":"Owner State","type":"string"},"request_status":{"default":"all","maxLength":8192,"title":"Request Status","type":"string"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"}},"title":"OwnershipViewSettings","type":"object"},"RegionViewSettings":{"additionalProperties":false,"properties":{"evidence":{"default":"all","maxLength":8192,"title":"Evidence","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"RegionViewSettings","type":"object"},"TimeViewSettings":{"additionalProperties":false,"properties":{"grants_only":{"default":false,"title":"Grants Only","type":"boolean"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"}},"title":"TimeViewSettings","type":"object"},"TopologyViewSettings":{"additionalProperties":false,"properties":{"evidence":{"default":"all","maxLength":8192,"title":"Evidence","type":"string"},"focused":{"default":"all","maxLength":8192,"title":"Focused","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"source":{"default":"all","maxLength":8192,"title":"Source","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"TopologyViewSettings","type":"object"},"WorkspaceSettings":{"additionalProperties":false,"properties":{"alerts":{"$ref":"#/$defs/AlertViewSettings","default":{"search":"","status":"all"}},"declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"federate_declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"federate_events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"federate_time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}},"federate_views":{"default":[],"items":{"$ref":"#/$defs/FederateWorkspaceSettings"},"maxItems":64,"title":"Federate Views","type":"array"},"federates":{"$ref":"#/$defs/FederateListSettings","default":{"basis":"all","search":""}},"federation":{"$ref":"#/$defs/FederationViewSettings","default":{"distribution":"event_types"}},"fom":{"$ref":"#/$defs/FomViewSettings","default":{"kind":"all","model":"all","search":"","selected":null,"status":"all"}},"interactions":{"$ref":"#/$defs/InteractionViewSettings","default":{"order":"all","search":"","status":"all"}},"mom":{"$ref":"#/$defs/MomViewSettings","default":{"counter":null,"scope":"all","search":"","service_status":"all"}},"object_charts":{"default":[],"items":{"$ref":"#/$defs/ObjectChartChoice"},"maxItems":64,"title":"Object Charts","type":"array"},"objects":{"$ref":"#/$defs/ObjectViewSettings","default":{"scope":null,"search":"","selected":"","status":"all"}},"ownership":{"$ref":"#/$defs/OwnershipViewSettings","default":{"owner_state":"all","request_status":"all","scope":"all","search":""}},"persistence":{"$ref":"#/$defs/CoordinationViewSettings","default":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"}},"regions":{"$ref":"#/$defs/RegionViewSettings","default":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"}},"selected_federate":{"default":"","maxLength":8192,"title":"Selected Federate","type":"string"},"synchronization":{"$ref":"#/$defs/CoordinationViewSettings","default":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"}},"time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}},"topology":{"$ref":"#/$defs/TopologyViewSettings","default":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"}},"view":{"default":"federation","enum":["federation","federates","objects","declarations","topology","interactions","ownership","regions","mom","fom","time","synchronization","persistence","events","inspectors","recordings","alerts","actions","diagnostics"],"title":"View","type":"string"},"window_seconds":{"default":300,"enum":[30,300,900,3600],"title":"Window Seconds","type":"integer"}},"title":"WorkspaceSettings","type":"object"}},"$id":"urn:umbra:observer:workspace:v1","$schema":"https://json-schema.org/draft/2020-12/schema","additionalProperties":false,"properties":{"name":{"maxLength":128,"minLength":1,"pattern":"[^\\u0009-\\u000d\\u0020\\u0085\\u00a0\\u1680\\u2000-\\u200a\\u2028\\u2029\\u202f\\u205f\\u3000]","title":"Name","type":"string"},"settings":{"$ref":"#/$defs/WorkspaceSettings","default":{"alerts":{"search":"","status":"all"},"declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"events":{"family":"","federate":"","search":"","source":"","type":""},"federate_declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"federate_events":{"family":"","federate":"","search":"","source":"","type":""},"federate_time":{"grants_only":false,"selected":null},"federate_views":[],"federates":{"basis":"all","search":""},"federation":{"distribution":"event_types"},"fom":{"kind":"all","model":"all","search":"","selected":null,"status":"all"},"interactions":{"order":"all","search":"","status":"all"},"mom":{"counter":null,"scope":"all","search":"","service_status":"all"},"object_charts":[],"objects":{"scope":null,"search":"","selected":"","status":"all"},"ownership":{"owner_state":"all","request_status":"all","scope":"all","search":""},"persistence":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"regions":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"},"selected_federate":"","synchronization":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"time":{"grants_only":false,"selected":null},"topology":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"},"view":"federation","window_seconds":300}},"workspace_version":{"const":"umbra-observer-workspace-v1","title":"Workspace Version","type":"string"}},"required":["workspace_version","name"],"title":"WorkspaceDocument","type":"object"};
const func0 = Object.prototype.hasOwnProperty;
const func63 = require("ajv/dist/runtime/ucs2length").default;
const pattern3 = new RegExp("[^\\u0009-\\u000d\\u0020\\u0085\\u00a0\\u1680\\u2000-\\u200a\\u2028\\u2029\\u202f\\u205f\\u3000]", "u");
const schema20 = {"additionalProperties":false,"properties":{"alerts":{"$ref":"#/$defs/AlertViewSettings","default":{"search":"","status":"all"}},"declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"federate_declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"federate_events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"federate_time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}},"federate_views":{"default":[],"items":{"$ref":"#/$defs/FederateWorkspaceSettings"},"maxItems":64,"title":"Federate Views","type":"array"},"federates":{"$ref":"#/$defs/FederateListSettings","default":{"basis":"all","search":""}},"federation":{"$ref":"#/$defs/FederationViewSettings","default":{"distribution":"event_types"}},"fom":{"$ref":"#/$defs/FomViewSettings","default":{"kind":"all","model":"all","search":"","selected":null,"status":"all"}},"interactions":{"$ref":"#/$defs/InteractionViewSettings","default":{"order":"all","search":"","status":"all"}},"mom":{"$ref":"#/$defs/MomViewSettings","default":{"counter":null,"scope":"all","search":"","service_status":"all"}},"object_charts":{"default":[],"items":{"$ref":"#/$defs/ObjectChartChoice"},"maxItems":64,"title":"Object Charts","type":"array"},"objects":{"$ref":"#/$defs/ObjectViewSettings","default":{"scope":null,"search":"","selected":"","status":"all"}},"ownership":{"$ref":"#/$defs/OwnershipViewSettings","default":{"owner_state":"all","request_status":"all","scope":"all","search":""}},"persistence":{"$ref":"#/$defs/CoordinationViewSettings","default":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"}},"regions":{"$ref":"#/$defs/RegionViewSettings","default":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"}},"selected_federate":{"default":"","maxLength":8192,"title":"Selected Federate","type":"string"},"synchronization":{"$ref":"#/$defs/CoordinationViewSettings","default":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"}},"time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}},"topology":{"$ref":"#/$defs/TopologyViewSettings","default":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"}},"view":{"default":"federation","enum":["federation","federates","objects","declarations","topology","interactions","ownership","regions","mom","fom","time","synchronization","persistence","events","inspectors","recordings","alerts","actions","diagnostics"],"title":"View","type":"string"},"window_seconds":{"default":300,"enum":[30,300,900,3600],"title":"Window Seconds","type":"integer"}},"title":"WorkspaceSettings","type":"object"};
const schema21 = {"additionalProperties":false,"properties":{"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"AlertViewSettings","type":"object"};

function validate54(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate54.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.search === undefined){
data.search = "";
}
if(data.status === undefined){
data.status = "all";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!((key0 === "search") || (key0 === "status"))){
validate54.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.search;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 2048){
validate54.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate54.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.status;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate54.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate54.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
}
}
}
else {
validate54.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate54.errors = vErrors;
return errors === 0;
}
validate54.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema22 = {"additionalProperties":false,"properties":{"direction":{"default":"all","maxLength":8192,"title":"Direction","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"perspective":{"default":"all","maxLength":8192,"title":"Perspective","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"DeclarationViewSettings","type":"object"};

function validate56(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate56.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.direction === undefined){
data.direction = "all";
}
if(data.kind === undefined){
data.kind = "all";
}
if(data.perspective === undefined){
data.perspective = "all";
}
if(data.search === undefined){
data.search = "";
}
if(data.status === undefined){
data.status = "all";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((((key0 === "direction") || (key0 === "kind")) || (key0 === "perspective")) || (key0 === "search")) || (key0 === "status"))){
validate56.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.direction;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
validate56.errors = [{instancePath:instancePath+"/direction",schemaPath:"#/properties/direction/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate56.errors = [{instancePath:instancePath+"/direction",schemaPath:"#/properties/direction/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.kind;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate56.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate56.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
if(valid0){
let data2 = data.perspective;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data2 === "string"){
if(func63(data2) > 8192){
validate56.errors = [{instancePath:instancePath+"/perspective",schemaPath:"#/properties/perspective/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate56.errors = [{instancePath:instancePath+"/perspective",schemaPath:"#/properties/perspective/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs6 === errors;
if(valid0){
let data3 = data.search;
const _errs8 = errors;
if(errors === _errs8){
if(typeof data3 === "string"){
if(func63(data3) > 2048){
validate56.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate56.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs8 === errors;
if(valid0){
let data4 = data.status;
const _errs10 = errors;
if(errors === _errs10){
if(typeof data4 === "string"){
if(func63(data4) > 8192){
validate56.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate56.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs10 === errors;
}
}
}
}
}
}
else {
validate56.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate56.errors = vErrors;
return errors === 0;
}
validate56.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema23 = {"additionalProperties":false,"properties":{"family":{"default":"","maxLength":8192,"title":"Family","type":"string"},"federate":{"default":"","maxLength":8192,"title":"Federate","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"source":{"default":"","maxLength":8192,"title":"Source","type":"string"},"type":{"default":"","maxLength":8192,"title":"Type","type":"string"}},"title":"EventViewSettings","type":"object"};

function validate58(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate58.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.family === undefined){
data.family = "";
}
if(data.federate === undefined){
data.federate = "";
}
if(data.search === undefined){
data.search = "";
}
if(data.source === undefined){
data.source = "";
}
if(data.type === undefined){
data.type = "";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((((key0 === "family") || (key0 === "federate")) || (key0 === "search")) || (key0 === "source")) || (key0 === "type"))){
validate58.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.family;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
validate58.errors = [{instancePath:instancePath+"/family",schemaPath:"#/properties/family/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate58.errors = [{instancePath:instancePath+"/family",schemaPath:"#/properties/family/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.federate;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate58.errors = [{instancePath:instancePath+"/federate",schemaPath:"#/properties/federate/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate58.errors = [{instancePath:instancePath+"/federate",schemaPath:"#/properties/federate/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
if(valid0){
let data2 = data.search;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data2 === "string"){
if(func63(data2) > 2048){
validate58.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate58.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs6 === errors;
if(valid0){
let data3 = data.source;
const _errs8 = errors;
if(errors === _errs8){
if(typeof data3 === "string"){
if(func63(data3) > 8192){
validate58.errors = [{instancePath:instancePath+"/source",schemaPath:"#/properties/source/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate58.errors = [{instancePath:instancePath+"/source",schemaPath:"#/properties/source/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs8 === errors;
if(valid0){
let data4 = data.type;
const _errs10 = errors;
if(errors === _errs10){
if(typeof data4 === "string"){
if(func63(data4) > 8192){
validate58.errors = [{instancePath:instancePath+"/type",schemaPath:"#/properties/type/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate58.errors = [{instancePath:instancePath+"/type",schemaPath:"#/properties/type/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs10 === errors;
}
}
}
}
}
}
else {
validate58.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate58.errors = vErrors;
return errors === 0;
}
validate58.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema24 = {"additionalProperties":false,"properties":{"grants_only":{"default":false,"title":"Grants Only","type":"boolean"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"}},"title":"TimeViewSettings","type":"object"};

function validate62(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate62.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.grants_only === undefined){
data.grants_only = false;
}
if(data.selected === undefined){
data.selected = null;
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!((key0 === "grants_only") || (key0 === "selected"))){
validate62.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
const _errs2 = errors;
if(typeof data.grants_only !== "boolean"){
validate62.errors = [{instancePath:instancePath+"/grants_only",schemaPath:"#/properties/grants_only/type",keyword:"type",params:{type: "boolean"},message:"must be boolean"}];
return false;
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.selected;
const _errs4 = errors;
const _errs5 = errors;
let valid1 = false;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
const err0 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
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
const err1 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err1];
}
else {
vErrors.push(err1);
}
errors++;
}
}
var _valid0 = _errs6 === errors;
valid1 = valid1 || _valid0;
const _errs8 = errors;
if(data1 !== null){
const err2 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err2];
}
else {
vErrors.push(err2);
}
errors++;
}
var _valid0 = _errs8 === errors;
valid1 = valid1 || _valid0;
if(!valid1){
const err3 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err3];
}
else {
vErrors.push(err3);
}
errors++;
validate62.errors = vErrors;
return false;
}
else {
errors = _errs5;
if(vErrors !== null){
if(_errs5){
vErrors.length = _errs5;
}
else {
vErrors = null;
}
}
}
var valid0 = _errs4 === errors;
}
}
}
else {
validate62.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate62.errors = vErrors;
return errors === 0;
}
validate62.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema25 = {"additionalProperties":false,"properties":{"declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"participant":{"maxLength":8192,"minLength":1,"title":"Participant","type":"string"},"time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}}},"required":["participant"],"title":"FederateWorkspaceSettings","type":"object"};

function validate64(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate64.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.declarations === undefined){
data.declarations = {"direction":"all","kind":"all","perspective":"all","search":"","status":"all"};
}
if(data.events === undefined){
data.events = {"family":"","federate":"","search":"","source":"","type":""};
}
if(data.time === undefined){
data.time = {"grants_only":false,"selected":null};
}
let missing0;
if(((data.participant === undefined) || (!(func0.call(data, "participant")))) && (missing0 = "participant")){
validate64.errors = [{instancePath,schemaPath:"#/required",keyword:"required",params:{missingProperty: missing0},message:"must have required property '"+missing0+"'"}];
return false;
}
else {
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!((((key0 === "declarations") || (key0 === "events")) || (key0 === "participant")) || (key0 === "time"))){
validate64.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
const _errs2 = errors;
if(!(validate56(data.declarations, {instancePath:instancePath+"/declarations",parentData:data,parentDataProperty:"declarations",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate56.errors : vErrors.concat(validate56.errors);
errors = vErrors.length;
}
var valid0 = _errs2 === errors;
if(valid0){
const _errs3 = errors;
if(!(validate58(data.events, {instancePath:instancePath+"/events",parentData:data,parentDataProperty:"events",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate58.errors : vErrors.concat(validate58.errors);
errors = vErrors.length;
}
var valid0 = _errs3 === errors;
if(valid0){
if(data.participant !== undefined && func0.call(data, "participant")){
let data2 = data.participant;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data2 === "string"){
if(func63(data2) > 8192){
validate64.errors = [{instancePath:instancePath+"/participant",schemaPath:"#/properties/participant/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
else {
if(func63(data2) < 1){
validate64.errors = [{instancePath:instancePath+"/participant",schemaPath:"#/properties/participant/minLength",keyword:"minLength",params:{limit: 1},message:"must NOT have fewer than 1 characters"}];
return false;
}
}
}
else {
validate64.errors = [{instancePath:instancePath+"/participant",schemaPath:"#/properties/participant/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
}
else {
var valid0 = true;
}
if(valid0){
const _errs6 = errors;
if(!(validate62(data.time, {instancePath:instancePath+"/time",parentData:data,parentDataProperty:"time",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate62.errors : vErrors.concat(validate62.errors);
errors = vErrors.length;
}
var valid0 = _errs6 === errors;
}
}
}
}
}
}
else {
validate64.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate64.errors = vErrors;
return errors === 0;
}
validate64.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema26 = {"additionalProperties":false,"properties":{"basis":{"default":"all","enum":["all","explicit-join","legacy-provenance","legacy-name"],"title":"Basis","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"}},"title":"FederateListSettings","type":"object"};

function validate69(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate69.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.basis === undefined){
data.basis = "all";
}
if(data.search === undefined){
data.search = "";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!((key0 === "basis") || (key0 === "search"))){
validate69.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.basis;
const _errs2 = errors;
if(typeof data0 !== "string"){
validate69.errors = [{instancePath:instancePath+"/basis",schemaPath:"#/properties/basis/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
if(!((((data0 === "all") || (data0 === "explicit-join")) || (data0 === "legacy-provenance")) || (data0 === "legacy-name"))){
validate69.errors = [{instancePath:instancePath+"/basis",schemaPath:"#/properties/basis/enum",keyword:"enum",params:{allowedValues: schema26.properties.basis.enum},message:"must be equal to one of the allowed values"}];
return false;
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.search;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 2048){
validate69.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate69.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
}
}
}
else {
validate69.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate69.errors = vErrors;
return errors === 0;
}
validate69.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema27 = {"additionalProperties":false,"properties":{"distribution":{"default":"event_types","enum":["event_types","sources","families"],"title":"Distribution","type":"string"}},"title":"FederationViewSettings","type":"object"};

function validate71(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate71.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.distribution === undefined){
data.distribution = "event_types";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(key0 === "distribution")){
validate71.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.distribution;
if(typeof data0 !== "string"){
validate71.errors = [{instancePath:instancePath+"/distribution",schemaPath:"#/properties/distribution/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
if(!(((data0 === "event_types") || (data0 === "sources")) || (data0 === "families"))){
validate71.errors = [{instancePath:instancePath+"/distribution",schemaPath:"#/properties/distribution/enum",keyword:"enum",params:{allowedValues: schema27.properties.distribution.enum},message:"must be equal to one of the allowed values"}];
return false;
}
}
}
else {
validate71.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate71.errors = vErrors;
return errors === 0;
}
validate71.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema28 = {"additionalProperties":false,"properties":{"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"model":{"default":"all","maxLength":8192,"title":"Model","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"FomViewSettings","type":"object"};

function validate73(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate73.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.kind === undefined){
data.kind = "all";
}
if(data.model === undefined){
data.model = "all";
}
if(data.search === undefined){
data.search = "";
}
if(data.selected === undefined){
data.selected = null;
}
if(data.status === undefined){
data.status = "all";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((((key0 === "kind") || (key0 === "model")) || (key0 === "search")) || (key0 === "selected")) || (key0 === "status"))){
validate73.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.kind;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
validate73.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate73.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.model;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate73.errors = [{instancePath:instancePath+"/model",schemaPath:"#/properties/model/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate73.errors = [{instancePath:instancePath+"/model",schemaPath:"#/properties/model/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
if(valid0){
let data2 = data.search;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data2 === "string"){
if(func63(data2) > 2048){
validate73.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate73.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs6 === errors;
if(valid0){
let data3 = data.selected;
const _errs8 = errors;
const _errs9 = errors;
let valid1 = false;
const _errs10 = errors;
if(errors === _errs10){
if(typeof data3 === "string"){
if(func63(data3) > 8192){
const err0 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
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
const err1 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err1];
}
else {
vErrors.push(err1);
}
errors++;
}
}
var _valid0 = _errs10 === errors;
valid1 = valid1 || _valid0;
const _errs12 = errors;
if(data3 !== null){
const err2 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err2];
}
else {
vErrors.push(err2);
}
errors++;
}
var _valid0 = _errs12 === errors;
valid1 = valid1 || _valid0;
if(!valid1){
const err3 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err3];
}
else {
vErrors.push(err3);
}
errors++;
validate73.errors = vErrors;
return false;
}
else {
errors = _errs9;
if(vErrors !== null){
if(_errs9){
vErrors.length = _errs9;
}
else {
vErrors = null;
}
}
}
var valid0 = _errs8 === errors;
if(valid0){
let data4 = data.status;
const _errs14 = errors;
if(errors === _errs14){
if(typeof data4 === "string"){
if(func63(data4) > 8192){
validate73.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate73.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs14 === errors;
}
}
}
}
}
}
else {
validate73.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate73.errors = vErrors;
return errors === 0;
}
validate73.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema29 = {"additionalProperties":false,"properties":{"order":{"default":"all","maxLength":8192,"title":"Order","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"InteractionViewSettings","type":"object"};

function validate75(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate75.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.order === undefined){
data.order = "all";
}
if(data.search === undefined){
data.search = "";
}
if(data.status === undefined){
data.status = "all";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((key0 === "order") || (key0 === "search")) || (key0 === "status"))){
validate75.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.order;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
validate75.errors = [{instancePath:instancePath+"/order",schemaPath:"#/properties/order/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate75.errors = [{instancePath:instancePath+"/order",schemaPath:"#/properties/order/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.search;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 2048){
validate75.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate75.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
if(valid0){
let data2 = data.status;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data2 === "string"){
if(func63(data2) > 8192){
validate75.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate75.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs6 === errors;
}
}
}
}
else {
validate75.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate75.errors = vErrors;
return errors === 0;
}
validate75.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema30 = {"additionalProperties":false,"properties":{"counter":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Counter"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"service_status":{"default":"all","maxLength":8192,"title":"Service Status","type":"string"}},"title":"MomViewSettings","type":"object"};

function validate77(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate77.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.counter === undefined){
data.counter = null;
}
if(data.scope === undefined){
data.scope = "all";
}
if(data.search === undefined){
data.search = "";
}
if(data.service_status === undefined){
data.service_status = "all";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!((((key0 === "counter") || (key0 === "scope")) || (key0 === "search")) || (key0 === "service_status"))){
validate77.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.counter;
const _errs2 = errors;
const _errs3 = errors;
let valid1 = false;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
const err0 = {instancePath:instancePath+"/counter",schemaPath:"#/properties/counter/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
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
const err1 = {instancePath:instancePath+"/counter",schemaPath:"#/properties/counter/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err1];
}
else {
vErrors.push(err1);
}
errors++;
}
}
var _valid0 = _errs4 === errors;
valid1 = valid1 || _valid0;
const _errs6 = errors;
if(data0 !== null){
const err2 = {instancePath:instancePath+"/counter",schemaPath:"#/properties/counter/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err2];
}
else {
vErrors.push(err2);
}
errors++;
}
var _valid0 = _errs6 === errors;
valid1 = valid1 || _valid0;
if(!valid1){
const err3 = {instancePath:instancePath+"/counter",schemaPath:"#/properties/counter/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err3];
}
else {
vErrors.push(err3);
}
errors++;
validate77.errors = vErrors;
return false;
}
else {
errors = _errs3;
if(vErrors !== null){
if(_errs3){
vErrors.length = _errs3;
}
else {
vErrors = null;
}
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.scope;
const _errs8 = errors;
if(errors === _errs8){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate77.errors = [{instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate77.errors = [{instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs8 === errors;
if(valid0){
let data2 = data.search;
const _errs10 = errors;
if(errors === _errs10){
if(typeof data2 === "string"){
if(func63(data2) > 2048){
validate77.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate77.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs10 === errors;
if(valid0){
let data3 = data.service_status;
const _errs12 = errors;
if(errors === _errs12){
if(typeof data3 === "string"){
if(func63(data3) > 8192){
validate77.errors = [{instancePath:instancePath+"/service_status",schemaPath:"#/properties/service_status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate77.errors = [{instancePath:instancePath+"/service_status",schemaPath:"#/properties/service_status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs12 === errors;
}
}
}
}
}
else {
validate77.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate77.errors = vErrors;
return errors === 0;
}
validate77.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema31 = {"additionalProperties":false,"properties":{"attribute":{"maxLength":8192,"minLength":1,"title":"Attribute","type":"string"},"object_key":{"maxLength":8192,"minLength":1,"title":"Object Key","type":"string"},"scope":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Scope"}},"required":["object_key","attribute"],"title":"ObjectChartChoice","type":"object"};

function validate79(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate79.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.scope === undefined){
data.scope = null;
}
let missing0;
if((((data.object_key === undefined) || (!(func0.call(data, "object_key")))) && (missing0 = "object_key")) || (((data.attribute === undefined) || (!(func0.call(data, "attribute")))) && (missing0 = "attribute"))){
validate79.errors = [{instancePath,schemaPath:"#/required",keyword:"required",params:{missingProperty: missing0},message:"must have required property '"+missing0+"'"}];
return false;
}
else {
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((key0 === "attribute") || (key0 === "object_key")) || (key0 === "scope"))){
validate79.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
if(data.attribute !== undefined && func0.call(data, "attribute")){
let data0 = data.attribute;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
validate79.errors = [{instancePath:instancePath+"/attribute",schemaPath:"#/properties/attribute/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
else {
if(func63(data0) < 1){
validate79.errors = [{instancePath:instancePath+"/attribute",schemaPath:"#/properties/attribute/minLength",keyword:"minLength",params:{limit: 1},message:"must NOT have fewer than 1 characters"}];
return false;
}
}
}
else {
validate79.errors = [{instancePath:instancePath+"/attribute",schemaPath:"#/properties/attribute/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
}
else {
var valid0 = true;
}
if(valid0){
if(data.object_key !== undefined && func0.call(data, "object_key")){
let data1 = data.object_key;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate79.errors = [{instancePath:instancePath+"/object_key",schemaPath:"#/properties/object_key/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
else {
if(func63(data1) < 1){
validate79.errors = [{instancePath:instancePath+"/object_key",schemaPath:"#/properties/object_key/minLength",keyword:"minLength",params:{limit: 1},message:"must NOT have fewer than 1 characters"}];
return false;
}
}
}
else {
validate79.errors = [{instancePath:instancePath+"/object_key",schemaPath:"#/properties/object_key/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
}
else {
var valid0 = true;
}
if(valid0){
let data2 = data.scope;
const _errs6 = errors;
const _errs7 = errors;
let valid1 = false;
const _errs8 = errors;
if(errors === _errs8){
if(typeof data2 === "string"){
if(func63(data2) > 8192){
const err0 = {instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
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
const err1 = {instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err1];
}
else {
vErrors.push(err1);
}
errors++;
}
}
var _valid0 = _errs8 === errors;
valid1 = valid1 || _valid0;
const _errs10 = errors;
if(data2 !== null){
const err2 = {instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err2];
}
else {
vErrors.push(err2);
}
errors++;
}
var _valid0 = _errs10 === errors;
valid1 = valid1 || _valid0;
if(!valid1){
const err3 = {instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err3];
}
else {
vErrors.push(err3);
}
errors++;
validate79.errors = vErrors;
return false;
}
else {
errors = _errs7;
if(vErrors !== null){
if(_errs7){
vErrors.length = _errs7;
}
else {
vErrors = null;
}
}
}
var valid0 = _errs6 === errors;
}
}
}
}
}
else {
validate79.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate79.errors = vErrors;
return errors === 0;
}
validate79.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema32 = {"additionalProperties":false,"properties":{"scope":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Scope"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"default":"","maxLength":8192,"title":"Selected","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"ObjectViewSettings","type":"object"};

function validate81(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate81.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.scope === undefined){
data.scope = null;
}
if(data.search === undefined){
data.search = "";
}
if(data.selected === undefined){
data.selected = "";
}
if(data.status === undefined){
data.status = "all";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!((((key0 === "scope") || (key0 === "search")) || (key0 === "selected")) || (key0 === "status"))){
validate81.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.scope;
const _errs2 = errors;
const _errs3 = errors;
let valid1 = false;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
const err0 = {instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
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
const err1 = {instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err1];
}
else {
vErrors.push(err1);
}
errors++;
}
}
var _valid0 = _errs4 === errors;
valid1 = valid1 || _valid0;
const _errs6 = errors;
if(data0 !== null){
const err2 = {instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err2];
}
else {
vErrors.push(err2);
}
errors++;
}
var _valid0 = _errs6 === errors;
valid1 = valid1 || _valid0;
if(!valid1){
const err3 = {instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err3];
}
else {
vErrors.push(err3);
}
errors++;
validate81.errors = vErrors;
return false;
}
else {
errors = _errs3;
if(vErrors !== null){
if(_errs3){
vErrors.length = _errs3;
}
else {
vErrors = null;
}
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.search;
const _errs8 = errors;
if(errors === _errs8){
if(typeof data1 === "string"){
if(func63(data1) > 2048){
validate81.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate81.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs8 === errors;
if(valid0){
let data2 = data.selected;
const _errs10 = errors;
if(errors === _errs10){
if(typeof data2 === "string"){
if(func63(data2) > 8192){
validate81.errors = [{instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate81.errors = [{instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs10 === errors;
if(valid0){
let data3 = data.status;
const _errs12 = errors;
if(errors === _errs12){
if(typeof data3 === "string"){
if(func63(data3) > 8192){
validate81.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate81.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs12 === errors;
}
}
}
}
}
else {
validate81.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate81.errors = vErrors;
return errors === 0;
}
validate81.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema33 = {"additionalProperties":false,"properties":{"owner_state":{"default":"all","maxLength":8192,"title":"Owner State","type":"string"},"request_status":{"default":"all","maxLength":8192,"title":"Request Status","type":"string"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"}},"title":"OwnershipViewSettings","type":"object"};

function validate83(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate83.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.owner_state === undefined){
data.owner_state = "all";
}
if(data.request_status === undefined){
data.request_status = "all";
}
if(data.scope === undefined){
data.scope = "all";
}
if(data.search === undefined){
data.search = "";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!((((key0 === "owner_state") || (key0 === "request_status")) || (key0 === "scope")) || (key0 === "search"))){
validate83.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.owner_state;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
validate83.errors = [{instancePath:instancePath+"/owner_state",schemaPath:"#/properties/owner_state/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate83.errors = [{instancePath:instancePath+"/owner_state",schemaPath:"#/properties/owner_state/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.request_status;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate83.errors = [{instancePath:instancePath+"/request_status",schemaPath:"#/properties/request_status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate83.errors = [{instancePath:instancePath+"/request_status",schemaPath:"#/properties/request_status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
if(valid0){
let data2 = data.scope;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data2 === "string"){
if(func63(data2) > 8192){
validate83.errors = [{instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate83.errors = [{instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs6 === errors;
if(valid0){
let data3 = data.search;
const _errs8 = errors;
if(errors === _errs8){
if(typeof data3 === "string"){
if(func63(data3) > 2048){
validate83.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate83.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs8 === errors;
}
}
}
}
}
else {
validate83.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate83.errors = vErrors;
return errors === 0;
}
validate83.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema34 = {"additionalProperties":false,"properties":{"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"phase":{"default":"all","maxLength":8192,"title":"Phase","type":"string"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"default":"all","maxLength":8192,"title":"Selected","type":"string"}},"title":"CoordinationViewSettings","type":"object"};

function validate85(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate85.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.kind === undefined){
data.kind = "all";
}
if(data.phase === undefined){
data.phase = "all";
}
if(data.scope === undefined){
data.scope = "all";
}
if(data.search === undefined){
data.search = "";
}
if(data.selected === undefined){
data.selected = "all";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((((key0 === "kind") || (key0 === "phase")) || (key0 === "scope")) || (key0 === "search")) || (key0 === "selected"))){
validate85.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.kind;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
validate85.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate85.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.phase;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate85.errors = [{instancePath:instancePath+"/phase",schemaPath:"#/properties/phase/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate85.errors = [{instancePath:instancePath+"/phase",schemaPath:"#/properties/phase/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
if(valid0){
let data2 = data.scope;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data2 === "string"){
if(func63(data2) > 8192){
validate85.errors = [{instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate85.errors = [{instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs6 === errors;
if(valid0){
let data3 = data.search;
const _errs8 = errors;
if(errors === _errs8){
if(typeof data3 === "string"){
if(func63(data3) > 2048){
validate85.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate85.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs8 === errors;
if(valid0){
let data4 = data.selected;
const _errs10 = errors;
if(errors === _errs10){
if(typeof data4 === "string"){
if(func63(data4) > 8192){
validate85.errors = [{instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate85.errors = [{instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs10 === errors;
}
}
}
}
}
}
else {
validate85.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate85.errors = vErrors;
return errors === 0;
}
validate85.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema35 = {"additionalProperties":false,"properties":{"evidence":{"default":"all","maxLength":8192,"title":"Evidence","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"RegionViewSettings","type":"object"};

function validate87(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate87.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.evidence === undefined){
data.evidence = "all";
}
if(data.kind === undefined){
data.kind = "all";
}
if(data.search === undefined){
data.search = "";
}
if(data.selected === undefined){
data.selected = null;
}
if(data.status === undefined){
data.status = "all";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((((key0 === "evidence") || (key0 === "kind")) || (key0 === "search")) || (key0 === "selected")) || (key0 === "status"))){
validate87.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.evidence;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
validate87.errors = [{instancePath:instancePath+"/evidence",schemaPath:"#/properties/evidence/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate87.errors = [{instancePath:instancePath+"/evidence",schemaPath:"#/properties/evidence/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.kind;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate87.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate87.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
if(valid0){
let data2 = data.search;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data2 === "string"){
if(func63(data2) > 2048){
validate87.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate87.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs6 === errors;
if(valid0){
let data3 = data.selected;
const _errs8 = errors;
const _errs9 = errors;
let valid1 = false;
const _errs10 = errors;
if(errors === _errs10){
if(typeof data3 === "string"){
if(func63(data3) > 8192){
const err0 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
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
const err1 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err1];
}
else {
vErrors.push(err1);
}
errors++;
}
}
var _valid0 = _errs10 === errors;
valid1 = valid1 || _valid0;
const _errs12 = errors;
if(data3 !== null){
const err2 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err2];
}
else {
vErrors.push(err2);
}
errors++;
}
var _valid0 = _errs12 === errors;
valid1 = valid1 || _valid0;
if(!valid1){
const err3 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err3];
}
else {
vErrors.push(err3);
}
errors++;
validate87.errors = vErrors;
return false;
}
else {
errors = _errs9;
if(vErrors !== null){
if(_errs9){
vErrors.length = _errs9;
}
else {
vErrors = null;
}
}
}
var valid0 = _errs8 === errors;
if(valid0){
let data4 = data.status;
const _errs14 = errors;
if(errors === _errs14){
if(typeof data4 === "string"){
if(func63(data4) > 8192){
validate87.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate87.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs14 === errors;
}
}
}
}
}
}
else {
validate87.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate87.errors = vErrors;
return errors === 0;
}
validate87.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema36 = {"additionalProperties":false,"properties":{"evidence":{"default":"all","maxLength":8192,"title":"Evidence","type":"string"},"focused":{"default":"all","maxLength":8192,"title":"Focused","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"source":{"default":"all","maxLength":8192,"title":"Source","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"TopologyViewSettings","type":"object"};

function validate91(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate91.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.evidence === undefined){
data.evidence = "all";
}
if(data.focused === undefined){
data.focused = "all";
}
if(data.kind === undefined){
data.kind = "all";
}
if(data.search === undefined){
data.search = "";
}
if(data.source === undefined){
data.source = "all";
}
if(data.status === undefined){
data.status = "all";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!((((((key0 === "evidence") || (key0 === "focused")) || (key0 === "kind")) || (key0 === "search")) || (key0 === "source")) || (key0 === "status"))){
validate91.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.evidence;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
validate91.errors = [{instancePath:instancePath+"/evidence",schemaPath:"#/properties/evidence/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate91.errors = [{instancePath:instancePath+"/evidence",schemaPath:"#/properties/evidence/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.focused;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate91.errors = [{instancePath:instancePath+"/focused",schemaPath:"#/properties/focused/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate91.errors = [{instancePath:instancePath+"/focused",schemaPath:"#/properties/focused/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
if(valid0){
let data2 = data.kind;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data2 === "string"){
if(func63(data2) > 8192){
validate91.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate91.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs6 === errors;
if(valid0){
let data3 = data.search;
const _errs8 = errors;
if(errors === _errs8){
if(typeof data3 === "string"){
if(func63(data3) > 2048){
validate91.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate91.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs8 === errors;
if(valid0){
let data4 = data.source;
const _errs10 = errors;
if(errors === _errs10){
if(typeof data4 === "string"){
if(func63(data4) > 8192){
validate91.errors = [{instancePath:instancePath+"/source",schemaPath:"#/properties/source/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate91.errors = [{instancePath:instancePath+"/source",schemaPath:"#/properties/source/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs10 === errors;
if(valid0){
let data5 = data.status;
const _errs12 = errors;
if(errors === _errs12){
if(typeof data5 === "string"){
if(func63(data5) > 8192){
validate91.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate91.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs12 === errors;
}
}
}
}
}
}
}
else {
validate91.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate91.errors = vErrors;
return errors === 0;
}
validate91.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};


function validate53(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate53.evaluated;
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
if(data.federate_views === undefined){
data.federate_views = [];
}
if(data.federates === undefined){
data.federates = {"basis":"all","search":""};
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
if(!(func0.call(schema20.properties, key0))){
validate53.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
const _errs2 = errors;
if(!(validate54(data.alerts, {instancePath:instancePath+"/alerts",parentData:data,parentDataProperty:"alerts",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate54.errors : vErrors.concat(validate54.errors);
errors = vErrors.length;
}
var valid0 = _errs2 === errors;
if(valid0){
const _errs3 = errors;
if(!(validate56(data.declarations, {instancePath:instancePath+"/declarations",parentData:data,parentDataProperty:"declarations",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate56.errors : vErrors.concat(validate56.errors);
errors = vErrors.length;
}
var valid0 = _errs3 === errors;
if(valid0){
const _errs4 = errors;
if(!(validate58(data.events, {instancePath:instancePath+"/events",parentData:data,parentDataProperty:"events",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate58.errors : vErrors.concat(validate58.errors);
errors = vErrors.length;
}
var valid0 = _errs4 === errors;
if(valid0){
const _errs5 = errors;
if(!(validate56(data.federate_declarations, {instancePath:instancePath+"/federate_declarations",parentData:data,parentDataProperty:"federate_declarations",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate56.errors : vErrors.concat(validate56.errors);
errors = vErrors.length;
}
var valid0 = _errs5 === errors;
if(valid0){
const _errs6 = errors;
if(!(validate58(data.federate_events, {instancePath:instancePath+"/federate_events",parentData:data,parentDataProperty:"federate_events",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate58.errors : vErrors.concat(validate58.errors);
errors = vErrors.length;
}
var valid0 = _errs6 === errors;
if(valid0){
const _errs7 = errors;
if(!(validate62(data.federate_time, {instancePath:instancePath+"/federate_time",parentData:data,parentDataProperty:"federate_time",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate62.errors : vErrors.concat(validate62.errors);
errors = vErrors.length;
}
var valid0 = _errs7 === errors;
if(valid0){
let data6 = data.federate_views;
const _errs8 = errors;
if(errors === _errs8){
if(Array.isArray(data6)){
if(data6.length > 64){
validate53.errors = [{instancePath:instancePath+"/federate_views",schemaPath:"#/properties/federate_views/maxItems",keyword:"maxItems",params:{limit: 64},message:"must NOT have more than 64 items"}];
return false;
}
else {
var valid1 = true;
const len0 = data6.length;
for(let i0=0; i0<len0; i0++){
const _errs10 = errors;
if(!(validate64(data6[i0], {instancePath:instancePath+"/federate_views/" + i0,parentData:data6,parentDataProperty:i0,rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate64.errors : vErrors.concat(validate64.errors);
errors = vErrors.length;
}
var valid1 = _errs10 === errors;
if(!valid1){
break;
}
}
}
}
else {
validate53.errors = [{instancePath:instancePath+"/federate_views",schemaPath:"#/properties/federate_views/type",keyword:"type",params:{type: "array"},message:"must be array"}];
return false;
}
}
var valid0 = _errs8 === errors;
if(valid0){
const _errs11 = errors;
if(!(validate69(data.federates, {instancePath:instancePath+"/federates",parentData:data,parentDataProperty:"federates",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate69.errors : vErrors.concat(validate69.errors);
errors = vErrors.length;
}
var valid0 = _errs11 === errors;
if(valid0){
const _errs12 = errors;
if(!(validate71(data.federation, {instancePath:instancePath+"/federation",parentData:data,parentDataProperty:"federation",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate71.errors : vErrors.concat(validate71.errors);
errors = vErrors.length;
}
var valid0 = _errs12 === errors;
if(valid0){
const _errs13 = errors;
if(!(validate73(data.fom, {instancePath:instancePath+"/fom",parentData:data,parentDataProperty:"fom",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate73.errors : vErrors.concat(validate73.errors);
errors = vErrors.length;
}
var valid0 = _errs13 === errors;
if(valid0){
const _errs14 = errors;
if(!(validate75(data.interactions, {instancePath:instancePath+"/interactions",parentData:data,parentDataProperty:"interactions",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate75.errors : vErrors.concat(validate75.errors);
errors = vErrors.length;
}
var valid0 = _errs14 === errors;
if(valid0){
const _errs15 = errors;
if(!(validate77(data.mom, {instancePath:instancePath+"/mom",parentData:data,parentDataProperty:"mom",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate77.errors : vErrors.concat(validate77.errors);
errors = vErrors.length;
}
var valid0 = _errs15 === errors;
if(valid0){
let data13 = data.object_charts;
const _errs16 = errors;
if(errors === _errs16){
if(Array.isArray(data13)){
if(data13.length > 64){
validate53.errors = [{instancePath:instancePath+"/object_charts",schemaPath:"#/properties/object_charts/maxItems",keyword:"maxItems",params:{limit: 64},message:"must NOT have more than 64 items"}];
return false;
}
else {
var valid2 = true;
const len1 = data13.length;
for(let i1=0; i1<len1; i1++){
const _errs18 = errors;
if(!(validate79(data13[i1], {instancePath:instancePath+"/object_charts/" + i1,parentData:data13,parentDataProperty:i1,rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate79.errors : vErrors.concat(validate79.errors);
errors = vErrors.length;
}
var valid2 = _errs18 === errors;
if(!valid2){
break;
}
}
}
}
else {
validate53.errors = [{instancePath:instancePath+"/object_charts",schemaPath:"#/properties/object_charts/type",keyword:"type",params:{type: "array"},message:"must be array"}];
return false;
}
}
var valid0 = _errs16 === errors;
if(valid0){
const _errs19 = errors;
if(!(validate81(data.objects, {instancePath:instancePath+"/objects",parentData:data,parentDataProperty:"objects",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate81.errors : vErrors.concat(validate81.errors);
errors = vErrors.length;
}
var valid0 = _errs19 === errors;
if(valid0){
const _errs20 = errors;
if(!(validate83(data.ownership, {instancePath:instancePath+"/ownership",parentData:data,parentDataProperty:"ownership",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate83.errors : vErrors.concat(validate83.errors);
errors = vErrors.length;
}
var valid0 = _errs20 === errors;
if(valid0){
const _errs21 = errors;
if(!(validate85(data.persistence, {instancePath:instancePath+"/persistence",parentData:data,parentDataProperty:"persistence",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate85.errors : vErrors.concat(validate85.errors);
errors = vErrors.length;
}
var valid0 = _errs21 === errors;
if(valid0){
const _errs22 = errors;
if(!(validate87(data.regions, {instancePath:instancePath+"/regions",parentData:data,parentDataProperty:"regions",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate87.errors : vErrors.concat(validate87.errors);
errors = vErrors.length;
}
var valid0 = _errs22 === errors;
if(valid0){
let data19 = data.selected_federate;
const _errs23 = errors;
if(errors === _errs23){
if(typeof data19 === "string"){
if(func63(data19) > 8192){
validate53.errors = [{instancePath:instancePath+"/selected_federate",schemaPath:"#/properties/selected_federate/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate53.errors = [{instancePath:instancePath+"/selected_federate",schemaPath:"#/properties/selected_federate/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs23 === errors;
if(valid0){
const _errs25 = errors;
if(!(validate85(data.synchronization, {instancePath:instancePath+"/synchronization",parentData:data,parentDataProperty:"synchronization",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate85.errors : vErrors.concat(validate85.errors);
errors = vErrors.length;
}
var valid0 = _errs25 === errors;
if(valid0){
const _errs26 = errors;
if(!(validate62(data.time, {instancePath:instancePath+"/time",parentData:data,parentDataProperty:"time",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate62.errors : vErrors.concat(validate62.errors);
errors = vErrors.length;
}
var valid0 = _errs26 === errors;
if(valid0){
const _errs27 = errors;
if(!(validate91(data.topology, {instancePath:instancePath+"/topology",parentData:data,parentDataProperty:"topology",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate91.errors : vErrors.concat(validate91.errors);
errors = vErrors.length;
}
var valid0 = _errs27 === errors;
if(valid0){
let data23 = data.view;
const _errs28 = errors;
if(typeof data23 !== "string"){
validate53.errors = [{instancePath:instancePath+"/view",schemaPath:"#/properties/view/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
if(!(((((((((((((((((((data23 === "federation") || (data23 === "federates")) || (data23 === "objects")) || (data23 === "declarations")) || (data23 === "topology")) || (data23 === "interactions")) || (data23 === "ownership")) || (data23 === "regions")) || (data23 === "mom")) || (data23 === "fom")) || (data23 === "time")) || (data23 === "synchronization")) || (data23 === "persistence")) || (data23 === "events")) || (data23 === "inspectors")) || (data23 === "recordings")) || (data23 === "alerts")) || (data23 === "actions")) || (data23 === "diagnostics"))){
validate53.errors = [{instancePath:instancePath+"/view",schemaPath:"#/properties/view/enum",keyword:"enum",params:{allowedValues: schema20.properties.view.enum},message:"must be equal to one of the allowed values"}];
return false;
}
var valid0 = _errs28 === errors;
if(valid0){
let data24 = data.window_seconds;
const _errs30 = errors;
if(!(((typeof data24 == "number") && (!(data24 % 1) && !isNaN(data24))) && (isFinite(data24)))){
validate53.errors = [{instancePath:instancePath+"/window_seconds",schemaPath:"#/properties/window_seconds/type",keyword:"type",params:{type: "integer"},message:"must be integer"}];
return false;
}
if(!((((data24 === 30) || (data24 === 300)) || (data24 === 900)) || (data24 === 3600))){
validate53.errors = [{instancePath:instancePath+"/window_seconds",schemaPath:"#/properties/window_seconds/enum",keyword:"enum",params:{allowedValues: schema20.properties.window_seconds.enum},message:"must be equal to one of the allowed values"}];
return false;
}
var valid0 = _errs30 === errors;
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
}
}
else {
validate53.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate53.errors = vErrors;
return errors === 0;
}
validate53.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};


function validate52(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
/*# sourceURL="urn:umbra:observer:workspace:v1" */;
let vErrors = null;
let errors = 0;
const evaluated0 = validate52.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.settings === undefined){
data.settings = {"alerts":{"search":"","status":"all"},"declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"events":{"family":"","federate":"","search":"","source":"","type":""},"federate_declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"federate_events":{"family":"","federate":"","search":"","source":"","type":""},"federate_time":{"grants_only":false,"selected":null},"federate_views":[],"federates":{"basis":"all","search":""},"federation":{"distribution":"event_types"},"fom":{"kind":"all","model":"all","search":"","selected":null,"status":"all"},"interactions":{"order":"all","search":"","status":"all"},"mom":{"counter":null,"scope":"all","search":"","service_status":"all"},"object_charts":[],"objects":{"scope":null,"search":"","selected":"","status":"all"},"ownership":{"owner_state":"all","request_status":"all","scope":"all","search":""},"persistence":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"regions":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"},"selected_federate":"","synchronization":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"time":{"grants_only":false,"selected":null},"topology":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"},"view":"federation","window_seconds":300};
}
let missing0;
if((((data.workspace_version === undefined) || (!(func0.call(data, "workspace_version")))) && (missing0 = "workspace_version")) || (((data.name === undefined) || (!(func0.call(data, "name")))) && (missing0 = "name"))){
validate52.errors = [{instancePath,schemaPath:"#/required",keyword:"required",params:{missingProperty: missing0},message:"must have required property '"+missing0+"'"}];
return false;
}
else {
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((key0 === "name") || (key0 === "settings")) || (key0 === "workspace_version"))){
validate52.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
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
validate52.errors = [{instancePath:instancePath+"/name",schemaPath:"#/properties/name/maxLength",keyword:"maxLength",params:{limit: 128},message:"must NOT have more than 128 characters"}];
return false;
}
else {
if(func63(data0) < 1){
validate52.errors = [{instancePath:instancePath+"/name",schemaPath:"#/properties/name/minLength",keyword:"minLength",params:{limit: 1},message:"must NOT have fewer than 1 characters"}];
return false;
}
else {
if(!pattern3.test(data0)){
validate52.errors = [{instancePath:instancePath+"/name",schemaPath:"#/properties/name/pattern",keyword:"pattern",params:{pattern: "[^\\u0009-\\u000d\\u0020\\u0085\\u00a0\\u1680\\u2000-\\u200a\\u2028\\u2029\\u202f\\u205f\\u3000]"},message:"must match pattern \""+"[^\\u0009-\\u000d\\u0020\\u0085\\u00a0\\u1680\\u2000-\\u200a\\u2028\\u2029\\u202f\\u205f\\u3000]"+"\""}];
return false;
}
}
}
}
else {
validate52.errors = [{instancePath:instancePath+"/name",schemaPath:"#/properties/name/type",keyword:"type",params:{type: "string"},message:"must be string"}];
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
if(!(validate53(data.settings, {instancePath:instancePath+"/settings",parentData:data,parentDataProperty:"settings",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate53.errors : vErrors.concat(validate53.errors);
errors = vErrors.length;
}
var valid0 = _errs4 === errors;
if(valid0){
if(data.workspace_version !== undefined && func0.call(data, "workspace_version")){
let data2 = data.workspace_version;
const _errs5 = errors;
if(typeof data2 !== "string"){
validate52.errors = [{instancePath:instancePath+"/workspace_version",schemaPath:"#/properties/workspace_version/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
if("umbra-observer-workspace-v1" !== data2){
validate52.errors = [{instancePath:instancePath+"/workspace_version",schemaPath:"#/properties/workspace_version/const",keyword:"const",params:{allowedValue: "umbra-observer-workspace-v1"},message:"must be equal to constant"}];
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
validate52.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate52.errors = vErrors;
return errors === 0;
}
validate52.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

exports.validateLibrary = validate94;
const schema37 = {"$defs":{"AlertViewSettings":{"additionalProperties":false,"properties":{"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"AlertViewSettings","type":"object"},"CoordinationViewSettings":{"additionalProperties":false,"properties":{"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"phase":{"default":"all","maxLength":8192,"title":"Phase","type":"string"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"default":"all","maxLength":8192,"title":"Selected","type":"string"}},"title":"CoordinationViewSettings","type":"object"},"DeclarationViewSettings":{"additionalProperties":false,"properties":{"direction":{"default":"all","maxLength":8192,"title":"Direction","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"perspective":{"default":"all","maxLength":8192,"title":"Perspective","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"DeclarationViewSettings","type":"object"},"EventViewSettings":{"additionalProperties":false,"properties":{"family":{"default":"","maxLength":8192,"title":"Family","type":"string"},"federate":{"default":"","maxLength":8192,"title":"Federate","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"source":{"default":"","maxLength":8192,"title":"Source","type":"string"},"type":{"default":"","maxLength":8192,"title":"Type","type":"string"}},"title":"EventViewSettings","type":"object"},"FederateListSettings":{"additionalProperties":false,"properties":{"basis":{"default":"all","enum":["all","explicit-join","legacy-provenance","legacy-name"],"title":"Basis","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"}},"title":"FederateListSettings","type":"object"},"FederateWorkspaceSettings":{"additionalProperties":false,"properties":{"declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"participant":{"maxLength":8192,"minLength":1,"title":"Participant","type":"string"},"time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}}},"required":["participant"],"title":"FederateWorkspaceSettings","type":"object"},"FederationViewSettings":{"additionalProperties":false,"properties":{"distribution":{"default":"event_types","enum":["event_types","sources","families"],"title":"Distribution","type":"string"}},"title":"FederationViewSettings","type":"object"},"FomViewSettings":{"additionalProperties":false,"properties":{"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"model":{"default":"all","maxLength":8192,"title":"Model","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"FomViewSettings","type":"object"},"InteractionViewSettings":{"additionalProperties":false,"properties":{"order":{"default":"all","maxLength":8192,"title":"Order","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"InteractionViewSettings","type":"object"},"MomViewSettings":{"additionalProperties":false,"properties":{"counter":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Counter"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"service_status":{"default":"all","maxLength":8192,"title":"Service Status","type":"string"}},"title":"MomViewSettings","type":"object"},"ObjectChartChoice":{"additionalProperties":false,"properties":{"attribute":{"maxLength":8192,"minLength":1,"title":"Attribute","type":"string"},"object_key":{"maxLength":8192,"minLength":1,"title":"Object Key","type":"string"},"scope":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Scope"}},"required":["object_key","attribute"],"title":"ObjectChartChoice","type":"object"},"ObjectViewSettings":{"additionalProperties":false,"properties":{"scope":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Scope"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"default":"","maxLength":8192,"title":"Selected","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"ObjectViewSettings","type":"object"},"OwnershipViewSettings":{"additionalProperties":false,"properties":{"owner_state":{"default":"all","maxLength":8192,"title":"Owner State","type":"string"},"request_status":{"default":"all","maxLength":8192,"title":"Request Status","type":"string"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"}},"title":"OwnershipViewSettings","type":"object"},"RegionViewSettings":{"additionalProperties":false,"properties":{"evidence":{"default":"all","maxLength":8192,"title":"Evidence","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"RegionViewSettings","type":"object"},"TimeViewSettings":{"additionalProperties":false,"properties":{"grants_only":{"default":false,"title":"Grants Only","type":"boolean"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"}},"title":"TimeViewSettings","type":"object"},"TopologyViewSettings":{"additionalProperties":false,"properties":{"evidence":{"default":"all","maxLength":8192,"title":"Evidence","type":"string"},"focused":{"default":"all","maxLength":8192,"title":"Focused","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"source":{"default":"all","maxLength":8192,"title":"Source","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"TopologyViewSettings","type":"object"},"WorkspaceDocument":{"additionalProperties":false,"properties":{"name":{"maxLength":128,"minLength":1,"pattern":"[^\\u0009-\\u000d\\u0020\\u0085\\u00a0\\u1680\\u2000-\\u200a\\u2028\\u2029\\u202f\\u205f\\u3000]","title":"Name","type":"string"},"settings":{"$ref":"#/$defs/WorkspaceSettings","default":{"alerts":{"search":"","status":"all"},"declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"events":{"family":"","federate":"","search":"","source":"","type":""},"federate_declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"federate_events":{"family":"","federate":"","search":"","source":"","type":""},"federate_time":{"grants_only":false,"selected":null},"federate_views":[],"federates":{"basis":"all","search":""},"federation":{"distribution":"event_types"},"fom":{"kind":"all","model":"all","search":"","selected":null,"status":"all"},"interactions":{"order":"all","search":"","status":"all"},"mom":{"counter":null,"scope":"all","search":"","service_status":"all"},"object_charts":[],"objects":{"scope":null,"search":"","selected":"","status":"all"},"ownership":{"owner_state":"all","request_status":"all","scope":"all","search":""},"persistence":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"regions":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"},"selected_federate":"","synchronization":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"time":{"grants_only":false,"selected":null},"topology":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"},"view":"federation","window_seconds":300}},"workspace_version":{"const":"umbra-observer-workspace-v1","title":"Workspace Version","type":"string"}},"required":["workspace_version","name"],"title":"WorkspaceDocument","type":"object"},"WorkspaceSettings":{"additionalProperties":false,"properties":{"alerts":{"$ref":"#/$defs/AlertViewSettings","default":{"search":"","status":"all"}},"declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"federate_declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"federate_events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"federate_time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}},"federate_views":{"default":[],"items":{"$ref":"#/$defs/FederateWorkspaceSettings"},"maxItems":64,"title":"Federate Views","type":"array"},"federates":{"$ref":"#/$defs/FederateListSettings","default":{"basis":"all","search":""}},"federation":{"$ref":"#/$defs/FederationViewSettings","default":{"distribution":"event_types"}},"fom":{"$ref":"#/$defs/FomViewSettings","default":{"kind":"all","model":"all","search":"","selected":null,"status":"all"}},"interactions":{"$ref":"#/$defs/InteractionViewSettings","default":{"order":"all","search":"","status":"all"}},"mom":{"$ref":"#/$defs/MomViewSettings","default":{"counter":null,"scope":"all","search":"","service_status":"all"}},"object_charts":{"default":[],"items":{"$ref":"#/$defs/ObjectChartChoice"},"maxItems":64,"title":"Object Charts","type":"array"},"objects":{"$ref":"#/$defs/ObjectViewSettings","default":{"scope":null,"search":"","selected":"","status":"all"}},"ownership":{"$ref":"#/$defs/OwnershipViewSettings","default":{"owner_state":"all","request_status":"all","scope":"all","search":""}},"persistence":{"$ref":"#/$defs/CoordinationViewSettings","default":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"}},"regions":{"$ref":"#/$defs/RegionViewSettings","default":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"}},"selected_federate":{"default":"","maxLength":8192,"title":"Selected Federate","type":"string"},"synchronization":{"$ref":"#/$defs/CoordinationViewSettings","default":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"}},"time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}},"topology":{"$ref":"#/$defs/TopologyViewSettings","default":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"}},"view":{"default":"federation","enum":["federation","federates","objects","declarations","topology","interactions","ownership","regions","mom","fom","time","synchronization","persistence","events","inspectors","recordings","alerts","actions","diagnostics"],"title":"View","type":"string"},"window_seconds":{"default":300,"enum":[30,300,900,3600],"title":"Window Seconds","type":"integer"}},"title":"WorkspaceSettings","type":"object"}},"$id":"urn:umbra:observer:workspace-library:v1","$schema":"https://json-schema.org/draft/2020-12/schema","additionalProperties":false,"properties":{"current":{"$ref":"#/$defs/WorkspaceSettings","default":{"alerts":{"search":"","status":"all"},"declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"events":{"family":"","federate":"","search":"","source":"","type":""},"federate_declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"federate_events":{"family":"","federate":"","search":"","source":"","type":""},"federate_time":{"grants_only":false,"selected":null},"federate_views":[],"federates":{"basis":"all","search":""},"federation":{"distribution":"event_types"},"fom":{"kind":"all","model":"all","search":"","selected":null,"status":"all"},"interactions":{"order":"all","search":"","status":"all"},"mom":{"counter":null,"scope":"all","search":"","service_status":"all"},"object_charts":[],"objects":{"scope":null,"search":"","selected":"","status":"all"},"ownership":{"owner_state":"all","request_status":"all","scope":"all","search":""},"persistence":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"regions":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"},"selected_federate":"","synchronization":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"time":{"grants_only":false,"selected":null},"topology":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"},"view":"federation","window_seconds":300}},"library_version":{"const":"umbra-observer-workspace-library-v1","title":"Library Version","type":"string"},"saved":{"default":[],"items":{"$ref":"#/$defs/WorkspaceDocument"},"maxItems":12,"title":"Saved","type":"array"}},"required":["library_version"],"title":"WorkspaceLibrary","type":"object"};
const schema38 = {"additionalProperties":false,"properties":{"alerts":{"$ref":"#/$defs/AlertViewSettings","default":{"search":"","status":"all"}},"declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"federate_declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"federate_events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"federate_time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}},"federate_views":{"default":[],"items":{"$ref":"#/$defs/FederateWorkspaceSettings"},"maxItems":64,"title":"Federate Views","type":"array"},"federates":{"$ref":"#/$defs/FederateListSettings","default":{"basis":"all","search":""}},"federation":{"$ref":"#/$defs/FederationViewSettings","default":{"distribution":"event_types"}},"fom":{"$ref":"#/$defs/FomViewSettings","default":{"kind":"all","model":"all","search":"","selected":null,"status":"all"}},"interactions":{"$ref":"#/$defs/InteractionViewSettings","default":{"order":"all","search":"","status":"all"}},"mom":{"$ref":"#/$defs/MomViewSettings","default":{"counter":null,"scope":"all","search":"","service_status":"all"}},"object_charts":{"default":[],"items":{"$ref":"#/$defs/ObjectChartChoice"},"maxItems":64,"title":"Object Charts","type":"array"},"objects":{"$ref":"#/$defs/ObjectViewSettings","default":{"scope":null,"search":"","selected":"","status":"all"}},"ownership":{"$ref":"#/$defs/OwnershipViewSettings","default":{"owner_state":"all","request_status":"all","scope":"all","search":""}},"persistence":{"$ref":"#/$defs/CoordinationViewSettings","default":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"}},"regions":{"$ref":"#/$defs/RegionViewSettings","default":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"}},"selected_federate":{"default":"","maxLength":8192,"title":"Selected Federate","type":"string"},"synchronization":{"$ref":"#/$defs/CoordinationViewSettings","default":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"}},"time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}},"topology":{"$ref":"#/$defs/TopologyViewSettings","default":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"}},"view":{"default":"federation","enum":["federation","federates","objects","declarations","topology","interactions","ownership","regions","mom","fom","time","synchronization","persistence","events","inspectors","recordings","alerts","actions","diagnostics"],"title":"View","type":"string"},"window_seconds":{"default":300,"enum":[30,300,900,3600],"title":"Window Seconds","type":"integer"}},"title":"WorkspaceSettings","type":"object"};
const schema39 = {"additionalProperties":false,"properties":{"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"AlertViewSettings","type":"object"};

function validate96(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate96.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.search === undefined){
data.search = "";
}
if(data.status === undefined){
data.status = "all";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!((key0 === "search") || (key0 === "status"))){
validate96.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.search;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 2048){
validate96.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate96.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.status;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate96.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate96.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
}
}
}
else {
validate96.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate96.errors = vErrors;
return errors === 0;
}
validate96.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema40 = {"additionalProperties":false,"properties":{"direction":{"default":"all","maxLength":8192,"title":"Direction","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"perspective":{"default":"all","maxLength":8192,"title":"Perspective","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"DeclarationViewSettings","type":"object"};

function validate98(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate98.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.direction === undefined){
data.direction = "all";
}
if(data.kind === undefined){
data.kind = "all";
}
if(data.perspective === undefined){
data.perspective = "all";
}
if(data.search === undefined){
data.search = "";
}
if(data.status === undefined){
data.status = "all";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((((key0 === "direction") || (key0 === "kind")) || (key0 === "perspective")) || (key0 === "search")) || (key0 === "status"))){
validate98.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.direction;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
validate98.errors = [{instancePath:instancePath+"/direction",schemaPath:"#/properties/direction/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate98.errors = [{instancePath:instancePath+"/direction",schemaPath:"#/properties/direction/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.kind;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate98.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate98.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
if(valid0){
let data2 = data.perspective;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data2 === "string"){
if(func63(data2) > 8192){
validate98.errors = [{instancePath:instancePath+"/perspective",schemaPath:"#/properties/perspective/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate98.errors = [{instancePath:instancePath+"/perspective",schemaPath:"#/properties/perspective/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs6 === errors;
if(valid0){
let data3 = data.search;
const _errs8 = errors;
if(errors === _errs8){
if(typeof data3 === "string"){
if(func63(data3) > 2048){
validate98.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate98.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs8 === errors;
if(valid0){
let data4 = data.status;
const _errs10 = errors;
if(errors === _errs10){
if(typeof data4 === "string"){
if(func63(data4) > 8192){
validate98.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate98.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs10 === errors;
}
}
}
}
}
}
else {
validate98.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate98.errors = vErrors;
return errors === 0;
}
validate98.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema41 = {"additionalProperties":false,"properties":{"family":{"default":"","maxLength":8192,"title":"Family","type":"string"},"federate":{"default":"","maxLength":8192,"title":"Federate","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"source":{"default":"","maxLength":8192,"title":"Source","type":"string"},"type":{"default":"","maxLength":8192,"title":"Type","type":"string"}},"title":"EventViewSettings","type":"object"};

function validate100(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate100.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.family === undefined){
data.family = "";
}
if(data.federate === undefined){
data.federate = "";
}
if(data.search === undefined){
data.search = "";
}
if(data.source === undefined){
data.source = "";
}
if(data.type === undefined){
data.type = "";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((((key0 === "family") || (key0 === "federate")) || (key0 === "search")) || (key0 === "source")) || (key0 === "type"))){
validate100.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.family;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
validate100.errors = [{instancePath:instancePath+"/family",schemaPath:"#/properties/family/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate100.errors = [{instancePath:instancePath+"/family",schemaPath:"#/properties/family/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.federate;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate100.errors = [{instancePath:instancePath+"/federate",schemaPath:"#/properties/federate/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate100.errors = [{instancePath:instancePath+"/federate",schemaPath:"#/properties/federate/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
if(valid0){
let data2 = data.search;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data2 === "string"){
if(func63(data2) > 2048){
validate100.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate100.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs6 === errors;
if(valid0){
let data3 = data.source;
const _errs8 = errors;
if(errors === _errs8){
if(typeof data3 === "string"){
if(func63(data3) > 8192){
validate100.errors = [{instancePath:instancePath+"/source",schemaPath:"#/properties/source/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate100.errors = [{instancePath:instancePath+"/source",schemaPath:"#/properties/source/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs8 === errors;
if(valid0){
let data4 = data.type;
const _errs10 = errors;
if(errors === _errs10){
if(typeof data4 === "string"){
if(func63(data4) > 8192){
validate100.errors = [{instancePath:instancePath+"/type",schemaPath:"#/properties/type/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate100.errors = [{instancePath:instancePath+"/type",schemaPath:"#/properties/type/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs10 === errors;
}
}
}
}
}
}
else {
validate100.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate100.errors = vErrors;
return errors === 0;
}
validate100.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema42 = {"additionalProperties":false,"properties":{"grants_only":{"default":false,"title":"Grants Only","type":"boolean"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"}},"title":"TimeViewSettings","type":"object"};

function validate104(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate104.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.grants_only === undefined){
data.grants_only = false;
}
if(data.selected === undefined){
data.selected = null;
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!((key0 === "grants_only") || (key0 === "selected"))){
validate104.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
const _errs2 = errors;
if(typeof data.grants_only !== "boolean"){
validate104.errors = [{instancePath:instancePath+"/grants_only",schemaPath:"#/properties/grants_only/type",keyword:"type",params:{type: "boolean"},message:"must be boolean"}];
return false;
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.selected;
const _errs4 = errors;
const _errs5 = errors;
let valid1 = false;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
const err0 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
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
const err1 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err1];
}
else {
vErrors.push(err1);
}
errors++;
}
}
var _valid0 = _errs6 === errors;
valid1 = valid1 || _valid0;
const _errs8 = errors;
if(data1 !== null){
const err2 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err2];
}
else {
vErrors.push(err2);
}
errors++;
}
var _valid0 = _errs8 === errors;
valid1 = valid1 || _valid0;
if(!valid1){
const err3 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err3];
}
else {
vErrors.push(err3);
}
errors++;
validate104.errors = vErrors;
return false;
}
else {
errors = _errs5;
if(vErrors !== null){
if(_errs5){
vErrors.length = _errs5;
}
else {
vErrors = null;
}
}
}
var valid0 = _errs4 === errors;
}
}
}
else {
validate104.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate104.errors = vErrors;
return errors === 0;
}
validate104.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema43 = {"additionalProperties":false,"properties":{"declarations":{"$ref":"#/$defs/DeclarationViewSettings","default":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"}},"events":{"$ref":"#/$defs/EventViewSettings","default":{"family":"","federate":"","search":"","source":"","type":""}},"participant":{"maxLength":8192,"minLength":1,"title":"Participant","type":"string"},"time":{"$ref":"#/$defs/TimeViewSettings","default":{"grants_only":false,"selected":null}}},"required":["participant"],"title":"FederateWorkspaceSettings","type":"object"};

function validate106(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate106.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.declarations === undefined){
data.declarations = {"direction":"all","kind":"all","perspective":"all","search":"","status":"all"};
}
if(data.events === undefined){
data.events = {"family":"","federate":"","search":"","source":"","type":""};
}
if(data.time === undefined){
data.time = {"grants_only":false,"selected":null};
}
let missing0;
if(((data.participant === undefined) || (!(func0.call(data, "participant")))) && (missing0 = "participant")){
validate106.errors = [{instancePath,schemaPath:"#/required",keyword:"required",params:{missingProperty: missing0},message:"must have required property '"+missing0+"'"}];
return false;
}
else {
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!((((key0 === "declarations") || (key0 === "events")) || (key0 === "participant")) || (key0 === "time"))){
validate106.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
const _errs2 = errors;
if(!(validate98(data.declarations, {instancePath:instancePath+"/declarations",parentData:data,parentDataProperty:"declarations",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate98.errors : vErrors.concat(validate98.errors);
errors = vErrors.length;
}
var valid0 = _errs2 === errors;
if(valid0){
const _errs3 = errors;
if(!(validate100(data.events, {instancePath:instancePath+"/events",parentData:data,parentDataProperty:"events",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate100.errors : vErrors.concat(validate100.errors);
errors = vErrors.length;
}
var valid0 = _errs3 === errors;
if(valid0){
if(data.participant !== undefined && func0.call(data, "participant")){
let data2 = data.participant;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data2 === "string"){
if(func63(data2) > 8192){
validate106.errors = [{instancePath:instancePath+"/participant",schemaPath:"#/properties/participant/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
else {
if(func63(data2) < 1){
validate106.errors = [{instancePath:instancePath+"/participant",schemaPath:"#/properties/participant/minLength",keyword:"minLength",params:{limit: 1},message:"must NOT have fewer than 1 characters"}];
return false;
}
}
}
else {
validate106.errors = [{instancePath:instancePath+"/participant",schemaPath:"#/properties/participant/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
}
else {
var valid0 = true;
}
if(valid0){
const _errs6 = errors;
if(!(validate104(data.time, {instancePath:instancePath+"/time",parentData:data,parentDataProperty:"time",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate104.errors : vErrors.concat(validate104.errors);
errors = vErrors.length;
}
var valid0 = _errs6 === errors;
}
}
}
}
}
}
else {
validate106.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate106.errors = vErrors;
return errors === 0;
}
validate106.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema44 = {"additionalProperties":false,"properties":{"basis":{"default":"all","enum":["all","explicit-join","legacy-provenance","legacy-name"],"title":"Basis","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"}},"title":"FederateListSettings","type":"object"};

function validate111(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate111.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.basis === undefined){
data.basis = "all";
}
if(data.search === undefined){
data.search = "";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!((key0 === "basis") || (key0 === "search"))){
validate111.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.basis;
const _errs2 = errors;
if(typeof data0 !== "string"){
validate111.errors = [{instancePath:instancePath+"/basis",schemaPath:"#/properties/basis/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
if(!((((data0 === "all") || (data0 === "explicit-join")) || (data0 === "legacy-provenance")) || (data0 === "legacy-name"))){
validate111.errors = [{instancePath:instancePath+"/basis",schemaPath:"#/properties/basis/enum",keyword:"enum",params:{allowedValues: schema44.properties.basis.enum},message:"must be equal to one of the allowed values"}];
return false;
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.search;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 2048){
validate111.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate111.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
}
}
}
else {
validate111.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate111.errors = vErrors;
return errors === 0;
}
validate111.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema45 = {"additionalProperties":false,"properties":{"distribution":{"default":"event_types","enum":["event_types","sources","families"],"title":"Distribution","type":"string"}},"title":"FederationViewSettings","type":"object"};

function validate113(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate113.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.distribution === undefined){
data.distribution = "event_types";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(key0 === "distribution")){
validate113.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.distribution;
if(typeof data0 !== "string"){
validate113.errors = [{instancePath:instancePath+"/distribution",schemaPath:"#/properties/distribution/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
if(!(((data0 === "event_types") || (data0 === "sources")) || (data0 === "families"))){
validate113.errors = [{instancePath:instancePath+"/distribution",schemaPath:"#/properties/distribution/enum",keyword:"enum",params:{allowedValues: schema45.properties.distribution.enum},message:"must be equal to one of the allowed values"}];
return false;
}
}
}
else {
validate113.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate113.errors = vErrors;
return errors === 0;
}
validate113.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema46 = {"additionalProperties":false,"properties":{"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"model":{"default":"all","maxLength":8192,"title":"Model","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"FomViewSettings","type":"object"};

function validate115(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate115.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.kind === undefined){
data.kind = "all";
}
if(data.model === undefined){
data.model = "all";
}
if(data.search === undefined){
data.search = "";
}
if(data.selected === undefined){
data.selected = null;
}
if(data.status === undefined){
data.status = "all";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((((key0 === "kind") || (key0 === "model")) || (key0 === "search")) || (key0 === "selected")) || (key0 === "status"))){
validate115.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.kind;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
validate115.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate115.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.model;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate115.errors = [{instancePath:instancePath+"/model",schemaPath:"#/properties/model/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate115.errors = [{instancePath:instancePath+"/model",schemaPath:"#/properties/model/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
if(valid0){
let data2 = data.search;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data2 === "string"){
if(func63(data2) > 2048){
validate115.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate115.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs6 === errors;
if(valid0){
let data3 = data.selected;
const _errs8 = errors;
const _errs9 = errors;
let valid1 = false;
const _errs10 = errors;
if(errors === _errs10){
if(typeof data3 === "string"){
if(func63(data3) > 8192){
const err0 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
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
const err1 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err1];
}
else {
vErrors.push(err1);
}
errors++;
}
}
var _valid0 = _errs10 === errors;
valid1 = valid1 || _valid0;
const _errs12 = errors;
if(data3 !== null){
const err2 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err2];
}
else {
vErrors.push(err2);
}
errors++;
}
var _valid0 = _errs12 === errors;
valid1 = valid1 || _valid0;
if(!valid1){
const err3 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err3];
}
else {
vErrors.push(err3);
}
errors++;
validate115.errors = vErrors;
return false;
}
else {
errors = _errs9;
if(vErrors !== null){
if(_errs9){
vErrors.length = _errs9;
}
else {
vErrors = null;
}
}
}
var valid0 = _errs8 === errors;
if(valid0){
let data4 = data.status;
const _errs14 = errors;
if(errors === _errs14){
if(typeof data4 === "string"){
if(func63(data4) > 8192){
validate115.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate115.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs14 === errors;
}
}
}
}
}
}
else {
validate115.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate115.errors = vErrors;
return errors === 0;
}
validate115.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema47 = {"additionalProperties":false,"properties":{"order":{"default":"all","maxLength":8192,"title":"Order","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"InteractionViewSettings","type":"object"};

function validate117(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate117.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.order === undefined){
data.order = "all";
}
if(data.search === undefined){
data.search = "";
}
if(data.status === undefined){
data.status = "all";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((key0 === "order") || (key0 === "search")) || (key0 === "status"))){
validate117.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.order;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
validate117.errors = [{instancePath:instancePath+"/order",schemaPath:"#/properties/order/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate117.errors = [{instancePath:instancePath+"/order",schemaPath:"#/properties/order/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.search;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 2048){
validate117.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate117.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
if(valid0){
let data2 = data.status;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data2 === "string"){
if(func63(data2) > 8192){
validate117.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate117.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs6 === errors;
}
}
}
}
else {
validate117.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate117.errors = vErrors;
return errors === 0;
}
validate117.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema48 = {"additionalProperties":false,"properties":{"counter":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Counter"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"service_status":{"default":"all","maxLength":8192,"title":"Service Status","type":"string"}},"title":"MomViewSettings","type":"object"};

function validate119(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate119.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.counter === undefined){
data.counter = null;
}
if(data.scope === undefined){
data.scope = "all";
}
if(data.search === undefined){
data.search = "";
}
if(data.service_status === undefined){
data.service_status = "all";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!((((key0 === "counter") || (key0 === "scope")) || (key0 === "search")) || (key0 === "service_status"))){
validate119.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.counter;
const _errs2 = errors;
const _errs3 = errors;
let valid1 = false;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
const err0 = {instancePath:instancePath+"/counter",schemaPath:"#/properties/counter/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
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
const err1 = {instancePath:instancePath+"/counter",schemaPath:"#/properties/counter/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err1];
}
else {
vErrors.push(err1);
}
errors++;
}
}
var _valid0 = _errs4 === errors;
valid1 = valid1 || _valid0;
const _errs6 = errors;
if(data0 !== null){
const err2 = {instancePath:instancePath+"/counter",schemaPath:"#/properties/counter/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err2];
}
else {
vErrors.push(err2);
}
errors++;
}
var _valid0 = _errs6 === errors;
valid1 = valid1 || _valid0;
if(!valid1){
const err3 = {instancePath:instancePath+"/counter",schemaPath:"#/properties/counter/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err3];
}
else {
vErrors.push(err3);
}
errors++;
validate119.errors = vErrors;
return false;
}
else {
errors = _errs3;
if(vErrors !== null){
if(_errs3){
vErrors.length = _errs3;
}
else {
vErrors = null;
}
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.scope;
const _errs8 = errors;
if(errors === _errs8){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate119.errors = [{instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate119.errors = [{instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs8 === errors;
if(valid0){
let data2 = data.search;
const _errs10 = errors;
if(errors === _errs10){
if(typeof data2 === "string"){
if(func63(data2) > 2048){
validate119.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate119.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs10 === errors;
if(valid0){
let data3 = data.service_status;
const _errs12 = errors;
if(errors === _errs12){
if(typeof data3 === "string"){
if(func63(data3) > 8192){
validate119.errors = [{instancePath:instancePath+"/service_status",schemaPath:"#/properties/service_status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate119.errors = [{instancePath:instancePath+"/service_status",schemaPath:"#/properties/service_status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs12 === errors;
}
}
}
}
}
else {
validate119.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate119.errors = vErrors;
return errors === 0;
}
validate119.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema49 = {"additionalProperties":false,"properties":{"attribute":{"maxLength":8192,"minLength":1,"title":"Attribute","type":"string"},"object_key":{"maxLength":8192,"minLength":1,"title":"Object Key","type":"string"},"scope":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Scope"}},"required":["object_key","attribute"],"title":"ObjectChartChoice","type":"object"};

function validate121(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate121.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.scope === undefined){
data.scope = null;
}
let missing0;
if((((data.object_key === undefined) || (!(func0.call(data, "object_key")))) && (missing0 = "object_key")) || (((data.attribute === undefined) || (!(func0.call(data, "attribute")))) && (missing0 = "attribute"))){
validate121.errors = [{instancePath,schemaPath:"#/required",keyword:"required",params:{missingProperty: missing0},message:"must have required property '"+missing0+"'"}];
return false;
}
else {
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((key0 === "attribute") || (key0 === "object_key")) || (key0 === "scope"))){
validate121.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
if(data.attribute !== undefined && func0.call(data, "attribute")){
let data0 = data.attribute;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
validate121.errors = [{instancePath:instancePath+"/attribute",schemaPath:"#/properties/attribute/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
else {
if(func63(data0) < 1){
validate121.errors = [{instancePath:instancePath+"/attribute",schemaPath:"#/properties/attribute/minLength",keyword:"minLength",params:{limit: 1},message:"must NOT have fewer than 1 characters"}];
return false;
}
}
}
else {
validate121.errors = [{instancePath:instancePath+"/attribute",schemaPath:"#/properties/attribute/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
}
else {
var valid0 = true;
}
if(valid0){
if(data.object_key !== undefined && func0.call(data, "object_key")){
let data1 = data.object_key;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate121.errors = [{instancePath:instancePath+"/object_key",schemaPath:"#/properties/object_key/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
else {
if(func63(data1) < 1){
validate121.errors = [{instancePath:instancePath+"/object_key",schemaPath:"#/properties/object_key/minLength",keyword:"minLength",params:{limit: 1},message:"must NOT have fewer than 1 characters"}];
return false;
}
}
}
else {
validate121.errors = [{instancePath:instancePath+"/object_key",schemaPath:"#/properties/object_key/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
}
else {
var valid0 = true;
}
if(valid0){
let data2 = data.scope;
const _errs6 = errors;
const _errs7 = errors;
let valid1 = false;
const _errs8 = errors;
if(errors === _errs8){
if(typeof data2 === "string"){
if(func63(data2) > 8192){
const err0 = {instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
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
const err1 = {instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err1];
}
else {
vErrors.push(err1);
}
errors++;
}
}
var _valid0 = _errs8 === errors;
valid1 = valid1 || _valid0;
const _errs10 = errors;
if(data2 !== null){
const err2 = {instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err2];
}
else {
vErrors.push(err2);
}
errors++;
}
var _valid0 = _errs10 === errors;
valid1 = valid1 || _valid0;
if(!valid1){
const err3 = {instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err3];
}
else {
vErrors.push(err3);
}
errors++;
validate121.errors = vErrors;
return false;
}
else {
errors = _errs7;
if(vErrors !== null){
if(_errs7){
vErrors.length = _errs7;
}
else {
vErrors = null;
}
}
}
var valid0 = _errs6 === errors;
}
}
}
}
}
else {
validate121.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate121.errors = vErrors;
return errors === 0;
}
validate121.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema50 = {"additionalProperties":false,"properties":{"scope":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Scope"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"default":"","maxLength":8192,"title":"Selected","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"ObjectViewSettings","type":"object"};

function validate123(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate123.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.scope === undefined){
data.scope = null;
}
if(data.search === undefined){
data.search = "";
}
if(data.selected === undefined){
data.selected = "";
}
if(data.status === undefined){
data.status = "all";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!((((key0 === "scope") || (key0 === "search")) || (key0 === "selected")) || (key0 === "status"))){
validate123.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.scope;
const _errs2 = errors;
const _errs3 = errors;
let valid1 = false;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
const err0 = {instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
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
const err1 = {instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err1];
}
else {
vErrors.push(err1);
}
errors++;
}
}
var _valid0 = _errs4 === errors;
valid1 = valid1 || _valid0;
const _errs6 = errors;
if(data0 !== null){
const err2 = {instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err2];
}
else {
vErrors.push(err2);
}
errors++;
}
var _valid0 = _errs6 === errors;
valid1 = valid1 || _valid0;
if(!valid1){
const err3 = {instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err3];
}
else {
vErrors.push(err3);
}
errors++;
validate123.errors = vErrors;
return false;
}
else {
errors = _errs3;
if(vErrors !== null){
if(_errs3){
vErrors.length = _errs3;
}
else {
vErrors = null;
}
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.search;
const _errs8 = errors;
if(errors === _errs8){
if(typeof data1 === "string"){
if(func63(data1) > 2048){
validate123.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate123.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs8 === errors;
if(valid0){
let data2 = data.selected;
const _errs10 = errors;
if(errors === _errs10){
if(typeof data2 === "string"){
if(func63(data2) > 8192){
validate123.errors = [{instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate123.errors = [{instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs10 === errors;
if(valid0){
let data3 = data.status;
const _errs12 = errors;
if(errors === _errs12){
if(typeof data3 === "string"){
if(func63(data3) > 8192){
validate123.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate123.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs12 === errors;
}
}
}
}
}
else {
validate123.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate123.errors = vErrors;
return errors === 0;
}
validate123.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema51 = {"additionalProperties":false,"properties":{"owner_state":{"default":"all","maxLength":8192,"title":"Owner State","type":"string"},"request_status":{"default":"all","maxLength":8192,"title":"Request Status","type":"string"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"}},"title":"OwnershipViewSettings","type":"object"};

function validate125(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate125.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.owner_state === undefined){
data.owner_state = "all";
}
if(data.request_status === undefined){
data.request_status = "all";
}
if(data.scope === undefined){
data.scope = "all";
}
if(data.search === undefined){
data.search = "";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!((((key0 === "owner_state") || (key0 === "request_status")) || (key0 === "scope")) || (key0 === "search"))){
validate125.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.owner_state;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
validate125.errors = [{instancePath:instancePath+"/owner_state",schemaPath:"#/properties/owner_state/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate125.errors = [{instancePath:instancePath+"/owner_state",schemaPath:"#/properties/owner_state/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.request_status;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate125.errors = [{instancePath:instancePath+"/request_status",schemaPath:"#/properties/request_status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate125.errors = [{instancePath:instancePath+"/request_status",schemaPath:"#/properties/request_status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
if(valid0){
let data2 = data.scope;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data2 === "string"){
if(func63(data2) > 8192){
validate125.errors = [{instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate125.errors = [{instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs6 === errors;
if(valid0){
let data3 = data.search;
const _errs8 = errors;
if(errors === _errs8){
if(typeof data3 === "string"){
if(func63(data3) > 2048){
validate125.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate125.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs8 === errors;
}
}
}
}
}
else {
validate125.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate125.errors = vErrors;
return errors === 0;
}
validate125.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema52 = {"additionalProperties":false,"properties":{"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"phase":{"default":"all","maxLength":8192,"title":"Phase","type":"string"},"scope":{"default":"all","maxLength":8192,"title":"Scope","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"default":"all","maxLength":8192,"title":"Selected","type":"string"}},"title":"CoordinationViewSettings","type":"object"};

function validate127(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate127.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.kind === undefined){
data.kind = "all";
}
if(data.phase === undefined){
data.phase = "all";
}
if(data.scope === undefined){
data.scope = "all";
}
if(data.search === undefined){
data.search = "";
}
if(data.selected === undefined){
data.selected = "all";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((((key0 === "kind") || (key0 === "phase")) || (key0 === "scope")) || (key0 === "search")) || (key0 === "selected"))){
validate127.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.kind;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
validate127.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate127.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.phase;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate127.errors = [{instancePath:instancePath+"/phase",schemaPath:"#/properties/phase/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate127.errors = [{instancePath:instancePath+"/phase",schemaPath:"#/properties/phase/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
if(valid0){
let data2 = data.scope;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data2 === "string"){
if(func63(data2) > 8192){
validate127.errors = [{instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate127.errors = [{instancePath:instancePath+"/scope",schemaPath:"#/properties/scope/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs6 === errors;
if(valid0){
let data3 = data.search;
const _errs8 = errors;
if(errors === _errs8){
if(typeof data3 === "string"){
if(func63(data3) > 2048){
validate127.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate127.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs8 === errors;
if(valid0){
let data4 = data.selected;
const _errs10 = errors;
if(errors === _errs10){
if(typeof data4 === "string"){
if(func63(data4) > 8192){
validate127.errors = [{instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate127.errors = [{instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs10 === errors;
}
}
}
}
}
}
else {
validate127.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate127.errors = vErrors;
return errors === 0;
}
validate127.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema53 = {"additionalProperties":false,"properties":{"evidence":{"default":"all","maxLength":8192,"title":"Evidence","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"selected":{"anyOf":[{"maxLength":8192,"type":"string"},{"type":"null"}],"default":null,"title":"Selected"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"RegionViewSettings","type":"object"};

function validate129(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate129.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.evidence === undefined){
data.evidence = "all";
}
if(data.kind === undefined){
data.kind = "all";
}
if(data.search === undefined){
data.search = "";
}
if(data.selected === undefined){
data.selected = null;
}
if(data.status === undefined){
data.status = "all";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((((key0 === "evidence") || (key0 === "kind")) || (key0 === "search")) || (key0 === "selected")) || (key0 === "status"))){
validate129.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.evidence;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
validate129.errors = [{instancePath:instancePath+"/evidence",schemaPath:"#/properties/evidence/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate129.errors = [{instancePath:instancePath+"/evidence",schemaPath:"#/properties/evidence/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.kind;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate129.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate129.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
if(valid0){
let data2 = data.search;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data2 === "string"){
if(func63(data2) > 2048){
validate129.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate129.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs6 === errors;
if(valid0){
let data3 = data.selected;
const _errs8 = errors;
const _errs9 = errors;
let valid1 = false;
const _errs10 = errors;
if(errors === _errs10){
if(typeof data3 === "string"){
if(func63(data3) > 8192){
const err0 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf/0/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"};
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
const err1 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf/0/type",keyword:"type",params:{type: "string"},message:"must be string"};
if(vErrors === null){
vErrors = [err1];
}
else {
vErrors.push(err1);
}
errors++;
}
}
var _valid0 = _errs10 === errors;
valid1 = valid1 || _valid0;
const _errs12 = errors;
if(data3 !== null){
const err2 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf/1/type",keyword:"type",params:{type: "null"},message:"must be null"};
if(vErrors === null){
vErrors = [err2];
}
else {
vErrors.push(err2);
}
errors++;
}
var _valid0 = _errs12 === errors;
valid1 = valid1 || _valid0;
if(!valid1){
const err3 = {instancePath:instancePath+"/selected",schemaPath:"#/properties/selected/anyOf",keyword:"anyOf",params:{},message:"must match a schema in anyOf"};
if(vErrors === null){
vErrors = [err3];
}
else {
vErrors.push(err3);
}
errors++;
validate129.errors = vErrors;
return false;
}
else {
errors = _errs9;
if(vErrors !== null){
if(_errs9){
vErrors.length = _errs9;
}
else {
vErrors = null;
}
}
}
var valid0 = _errs8 === errors;
if(valid0){
let data4 = data.status;
const _errs14 = errors;
if(errors === _errs14){
if(typeof data4 === "string"){
if(func63(data4) > 8192){
validate129.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate129.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs14 === errors;
}
}
}
}
}
}
else {
validate129.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate129.errors = vErrors;
return errors === 0;
}
validate129.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema54 = {"additionalProperties":false,"properties":{"evidence":{"default":"all","maxLength":8192,"title":"Evidence","type":"string"},"focused":{"default":"all","maxLength":8192,"title":"Focused","type":"string"},"kind":{"default":"all","maxLength":8192,"title":"Kind","type":"string"},"search":{"default":"","maxLength":2048,"title":"Search","type":"string"},"source":{"default":"all","maxLength":8192,"title":"Source","type":"string"},"status":{"default":"all","maxLength":8192,"title":"Status","type":"string"}},"title":"TopologyViewSettings","type":"object"};

function validate133(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate133.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.evidence === undefined){
data.evidence = "all";
}
if(data.focused === undefined){
data.focused = "all";
}
if(data.kind === undefined){
data.kind = "all";
}
if(data.search === undefined){
data.search = "";
}
if(data.source === undefined){
data.source = "all";
}
if(data.status === undefined){
data.status = "all";
}
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!((((((key0 === "evidence") || (key0 === "focused")) || (key0 === "kind")) || (key0 === "search")) || (key0 === "source")) || (key0 === "status"))){
validate133.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
let data0 = data.evidence;
const _errs2 = errors;
if(errors === _errs2){
if(typeof data0 === "string"){
if(func63(data0) > 8192){
validate133.errors = [{instancePath:instancePath+"/evidence",schemaPath:"#/properties/evidence/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate133.errors = [{instancePath:instancePath+"/evidence",schemaPath:"#/properties/evidence/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs2 === errors;
if(valid0){
let data1 = data.focused;
const _errs4 = errors;
if(errors === _errs4){
if(typeof data1 === "string"){
if(func63(data1) > 8192){
validate133.errors = [{instancePath:instancePath+"/focused",schemaPath:"#/properties/focused/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate133.errors = [{instancePath:instancePath+"/focused",schemaPath:"#/properties/focused/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs4 === errors;
if(valid0){
let data2 = data.kind;
const _errs6 = errors;
if(errors === _errs6){
if(typeof data2 === "string"){
if(func63(data2) > 8192){
validate133.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate133.errors = [{instancePath:instancePath+"/kind",schemaPath:"#/properties/kind/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs6 === errors;
if(valid0){
let data3 = data.search;
const _errs8 = errors;
if(errors === _errs8){
if(typeof data3 === "string"){
if(func63(data3) > 2048){
validate133.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/maxLength",keyword:"maxLength",params:{limit: 2048},message:"must NOT have more than 2048 characters"}];
return false;
}
}
else {
validate133.errors = [{instancePath:instancePath+"/search",schemaPath:"#/properties/search/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs8 === errors;
if(valid0){
let data4 = data.source;
const _errs10 = errors;
if(errors === _errs10){
if(typeof data4 === "string"){
if(func63(data4) > 8192){
validate133.errors = [{instancePath:instancePath+"/source",schemaPath:"#/properties/source/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate133.errors = [{instancePath:instancePath+"/source",schemaPath:"#/properties/source/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs10 === errors;
if(valid0){
let data5 = data.status;
const _errs12 = errors;
if(errors === _errs12){
if(typeof data5 === "string"){
if(func63(data5) > 8192){
validate133.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate133.errors = [{instancePath:instancePath+"/status",schemaPath:"#/properties/status/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs12 === errors;
}
}
}
}
}
}
}
else {
validate133.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate133.errors = vErrors;
return errors === 0;
}
validate133.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};


function validate95(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate95.evaluated;
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
if(data.federate_views === undefined){
data.federate_views = [];
}
if(data.federates === undefined){
data.federates = {"basis":"all","search":""};
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
if(!(func0.call(schema38.properties, key0))){
validate95.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
const _errs2 = errors;
if(!(validate96(data.alerts, {instancePath:instancePath+"/alerts",parentData:data,parentDataProperty:"alerts",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate96.errors : vErrors.concat(validate96.errors);
errors = vErrors.length;
}
var valid0 = _errs2 === errors;
if(valid0){
const _errs3 = errors;
if(!(validate98(data.declarations, {instancePath:instancePath+"/declarations",parentData:data,parentDataProperty:"declarations",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate98.errors : vErrors.concat(validate98.errors);
errors = vErrors.length;
}
var valid0 = _errs3 === errors;
if(valid0){
const _errs4 = errors;
if(!(validate100(data.events, {instancePath:instancePath+"/events",parentData:data,parentDataProperty:"events",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate100.errors : vErrors.concat(validate100.errors);
errors = vErrors.length;
}
var valid0 = _errs4 === errors;
if(valid0){
const _errs5 = errors;
if(!(validate98(data.federate_declarations, {instancePath:instancePath+"/federate_declarations",parentData:data,parentDataProperty:"federate_declarations",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate98.errors : vErrors.concat(validate98.errors);
errors = vErrors.length;
}
var valid0 = _errs5 === errors;
if(valid0){
const _errs6 = errors;
if(!(validate100(data.federate_events, {instancePath:instancePath+"/federate_events",parentData:data,parentDataProperty:"federate_events",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate100.errors : vErrors.concat(validate100.errors);
errors = vErrors.length;
}
var valid0 = _errs6 === errors;
if(valid0){
const _errs7 = errors;
if(!(validate104(data.federate_time, {instancePath:instancePath+"/federate_time",parentData:data,parentDataProperty:"federate_time",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate104.errors : vErrors.concat(validate104.errors);
errors = vErrors.length;
}
var valid0 = _errs7 === errors;
if(valid0){
let data6 = data.federate_views;
const _errs8 = errors;
if(errors === _errs8){
if(Array.isArray(data6)){
if(data6.length > 64){
validate95.errors = [{instancePath:instancePath+"/federate_views",schemaPath:"#/properties/federate_views/maxItems",keyword:"maxItems",params:{limit: 64},message:"must NOT have more than 64 items"}];
return false;
}
else {
var valid1 = true;
const len0 = data6.length;
for(let i0=0; i0<len0; i0++){
const _errs10 = errors;
if(!(validate106(data6[i0], {instancePath:instancePath+"/federate_views/" + i0,parentData:data6,parentDataProperty:i0,rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate106.errors : vErrors.concat(validate106.errors);
errors = vErrors.length;
}
var valid1 = _errs10 === errors;
if(!valid1){
break;
}
}
}
}
else {
validate95.errors = [{instancePath:instancePath+"/federate_views",schemaPath:"#/properties/federate_views/type",keyword:"type",params:{type: "array"},message:"must be array"}];
return false;
}
}
var valid0 = _errs8 === errors;
if(valid0){
const _errs11 = errors;
if(!(validate111(data.federates, {instancePath:instancePath+"/federates",parentData:data,parentDataProperty:"federates",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate111.errors : vErrors.concat(validate111.errors);
errors = vErrors.length;
}
var valid0 = _errs11 === errors;
if(valid0){
const _errs12 = errors;
if(!(validate113(data.federation, {instancePath:instancePath+"/federation",parentData:data,parentDataProperty:"federation",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate113.errors : vErrors.concat(validate113.errors);
errors = vErrors.length;
}
var valid0 = _errs12 === errors;
if(valid0){
const _errs13 = errors;
if(!(validate115(data.fom, {instancePath:instancePath+"/fom",parentData:data,parentDataProperty:"fom",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate115.errors : vErrors.concat(validate115.errors);
errors = vErrors.length;
}
var valid0 = _errs13 === errors;
if(valid0){
const _errs14 = errors;
if(!(validate117(data.interactions, {instancePath:instancePath+"/interactions",parentData:data,parentDataProperty:"interactions",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate117.errors : vErrors.concat(validate117.errors);
errors = vErrors.length;
}
var valid0 = _errs14 === errors;
if(valid0){
const _errs15 = errors;
if(!(validate119(data.mom, {instancePath:instancePath+"/mom",parentData:data,parentDataProperty:"mom",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate119.errors : vErrors.concat(validate119.errors);
errors = vErrors.length;
}
var valid0 = _errs15 === errors;
if(valid0){
let data13 = data.object_charts;
const _errs16 = errors;
if(errors === _errs16){
if(Array.isArray(data13)){
if(data13.length > 64){
validate95.errors = [{instancePath:instancePath+"/object_charts",schemaPath:"#/properties/object_charts/maxItems",keyword:"maxItems",params:{limit: 64},message:"must NOT have more than 64 items"}];
return false;
}
else {
var valid2 = true;
const len1 = data13.length;
for(let i1=0; i1<len1; i1++){
const _errs18 = errors;
if(!(validate121(data13[i1], {instancePath:instancePath+"/object_charts/" + i1,parentData:data13,parentDataProperty:i1,rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate121.errors : vErrors.concat(validate121.errors);
errors = vErrors.length;
}
var valid2 = _errs18 === errors;
if(!valid2){
break;
}
}
}
}
else {
validate95.errors = [{instancePath:instancePath+"/object_charts",schemaPath:"#/properties/object_charts/type",keyword:"type",params:{type: "array"},message:"must be array"}];
return false;
}
}
var valid0 = _errs16 === errors;
if(valid0){
const _errs19 = errors;
if(!(validate123(data.objects, {instancePath:instancePath+"/objects",parentData:data,parentDataProperty:"objects",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate123.errors : vErrors.concat(validate123.errors);
errors = vErrors.length;
}
var valid0 = _errs19 === errors;
if(valid0){
const _errs20 = errors;
if(!(validate125(data.ownership, {instancePath:instancePath+"/ownership",parentData:data,parentDataProperty:"ownership",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate125.errors : vErrors.concat(validate125.errors);
errors = vErrors.length;
}
var valid0 = _errs20 === errors;
if(valid0){
const _errs21 = errors;
if(!(validate127(data.persistence, {instancePath:instancePath+"/persistence",parentData:data,parentDataProperty:"persistence",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate127.errors : vErrors.concat(validate127.errors);
errors = vErrors.length;
}
var valid0 = _errs21 === errors;
if(valid0){
const _errs22 = errors;
if(!(validate129(data.regions, {instancePath:instancePath+"/regions",parentData:data,parentDataProperty:"regions",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate129.errors : vErrors.concat(validate129.errors);
errors = vErrors.length;
}
var valid0 = _errs22 === errors;
if(valid0){
let data19 = data.selected_federate;
const _errs23 = errors;
if(errors === _errs23){
if(typeof data19 === "string"){
if(func63(data19) > 8192){
validate95.errors = [{instancePath:instancePath+"/selected_federate",schemaPath:"#/properties/selected_federate/maxLength",keyword:"maxLength",params:{limit: 8192},message:"must NOT have more than 8192 characters"}];
return false;
}
}
else {
validate95.errors = [{instancePath:instancePath+"/selected_federate",schemaPath:"#/properties/selected_federate/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
}
var valid0 = _errs23 === errors;
if(valid0){
const _errs25 = errors;
if(!(validate127(data.synchronization, {instancePath:instancePath+"/synchronization",parentData:data,parentDataProperty:"synchronization",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate127.errors : vErrors.concat(validate127.errors);
errors = vErrors.length;
}
var valid0 = _errs25 === errors;
if(valid0){
const _errs26 = errors;
if(!(validate104(data.time, {instancePath:instancePath+"/time",parentData:data,parentDataProperty:"time",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate104.errors : vErrors.concat(validate104.errors);
errors = vErrors.length;
}
var valid0 = _errs26 === errors;
if(valid0){
const _errs27 = errors;
if(!(validate133(data.topology, {instancePath:instancePath+"/topology",parentData:data,parentDataProperty:"topology",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate133.errors : vErrors.concat(validate133.errors);
errors = vErrors.length;
}
var valid0 = _errs27 === errors;
if(valid0){
let data23 = data.view;
const _errs28 = errors;
if(typeof data23 !== "string"){
validate95.errors = [{instancePath:instancePath+"/view",schemaPath:"#/properties/view/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
if(!(((((((((((((((((((data23 === "federation") || (data23 === "federates")) || (data23 === "objects")) || (data23 === "declarations")) || (data23 === "topology")) || (data23 === "interactions")) || (data23 === "ownership")) || (data23 === "regions")) || (data23 === "mom")) || (data23 === "fom")) || (data23 === "time")) || (data23 === "synchronization")) || (data23 === "persistence")) || (data23 === "events")) || (data23 === "inspectors")) || (data23 === "recordings")) || (data23 === "alerts")) || (data23 === "actions")) || (data23 === "diagnostics"))){
validate95.errors = [{instancePath:instancePath+"/view",schemaPath:"#/properties/view/enum",keyword:"enum",params:{allowedValues: schema38.properties.view.enum},message:"must be equal to one of the allowed values"}];
return false;
}
var valid0 = _errs28 === errors;
if(valid0){
let data24 = data.window_seconds;
const _errs30 = errors;
if(!(((typeof data24 == "number") && (!(data24 % 1) && !isNaN(data24))) && (isFinite(data24)))){
validate95.errors = [{instancePath:instancePath+"/window_seconds",schemaPath:"#/properties/window_seconds/type",keyword:"type",params:{type: "integer"},message:"must be integer"}];
return false;
}
if(!((((data24 === 30) || (data24 === 300)) || (data24 === 900)) || (data24 === 3600))){
validate95.errors = [{instancePath:instancePath+"/window_seconds",schemaPath:"#/properties/window_seconds/enum",keyword:"enum",params:{allowedValues: schema38.properties.window_seconds.enum},message:"must be equal to one of the allowed values"}];
return false;
}
var valid0 = _errs30 === errors;
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
}
}
else {
validate95.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate95.errors = vErrors;
return errors === 0;
}
validate95.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};

const schema55 = {"additionalProperties":false,"properties":{"name":{"maxLength":128,"minLength":1,"pattern":"[^\\u0009-\\u000d\\u0020\\u0085\\u00a0\\u1680\\u2000-\\u200a\\u2028\\u2029\\u202f\\u205f\\u3000]","title":"Name","type":"string"},"settings":{"$ref":"#/$defs/WorkspaceSettings","default":{"alerts":{"search":"","status":"all"},"declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"events":{"family":"","federate":"","search":"","source":"","type":""},"federate_declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"federate_events":{"family":"","federate":"","search":"","source":"","type":""},"federate_time":{"grants_only":false,"selected":null},"federate_views":[],"federates":{"basis":"all","search":""},"federation":{"distribution":"event_types"},"fom":{"kind":"all","model":"all","search":"","selected":null,"status":"all"},"interactions":{"order":"all","search":"","status":"all"},"mom":{"counter":null,"scope":"all","search":"","service_status":"all"},"object_charts":[],"objects":{"scope":null,"search":"","selected":"","status":"all"},"ownership":{"owner_state":"all","request_status":"all","scope":"all","search":""},"persistence":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"regions":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"},"selected_federate":"","synchronization":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"time":{"grants_only":false,"selected":null},"topology":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"},"view":"federation","window_seconds":300}},"workspace_version":{"const":"umbra-observer-workspace-v1","title":"Workspace Version","type":"string"}},"required":["workspace_version","name"],"title":"WorkspaceDocument","type":"object"};

function validate136(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
let vErrors = null;
let errors = 0;
const evaluated0 = validate136.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.settings === undefined){
data.settings = {"alerts":{"search":"","status":"all"},"declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"events":{"family":"","federate":"","search":"","source":"","type":""},"federate_declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"federate_events":{"family":"","federate":"","search":"","source":"","type":""},"federate_time":{"grants_only":false,"selected":null},"federate_views":[],"federates":{"basis":"all","search":""},"federation":{"distribution":"event_types"},"fom":{"kind":"all","model":"all","search":"","selected":null,"status":"all"},"interactions":{"order":"all","search":"","status":"all"},"mom":{"counter":null,"scope":"all","search":"","service_status":"all"},"object_charts":[],"objects":{"scope":null,"search":"","selected":"","status":"all"},"ownership":{"owner_state":"all","request_status":"all","scope":"all","search":""},"persistence":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"regions":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"},"selected_federate":"","synchronization":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"time":{"grants_only":false,"selected":null},"topology":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"},"view":"federation","window_seconds":300};
}
let missing0;
if((((data.workspace_version === undefined) || (!(func0.call(data, "workspace_version")))) && (missing0 = "workspace_version")) || (((data.name === undefined) || (!(func0.call(data, "name")))) && (missing0 = "name"))){
validate136.errors = [{instancePath,schemaPath:"#/required",keyword:"required",params:{missingProperty: missing0},message:"must have required property '"+missing0+"'"}];
return false;
}
else {
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((key0 === "name") || (key0 === "settings")) || (key0 === "workspace_version"))){
validate136.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
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
validate136.errors = [{instancePath:instancePath+"/name",schemaPath:"#/properties/name/maxLength",keyword:"maxLength",params:{limit: 128},message:"must NOT have more than 128 characters"}];
return false;
}
else {
if(func63(data0) < 1){
validate136.errors = [{instancePath:instancePath+"/name",schemaPath:"#/properties/name/minLength",keyword:"minLength",params:{limit: 1},message:"must NOT have fewer than 1 characters"}];
return false;
}
else {
if(!pattern3.test(data0)){
validate136.errors = [{instancePath:instancePath+"/name",schemaPath:"#/properties/name/pattern",keyword:"pattern",params:{pattern: "[^\\u0009-\\u000d\\u0020\\u0085\\u00a0\\u1680\\u2000-\\u200a\\u2028\\u2029\\u202f\\u205f\\u3000]"},message:"must match pattern \""+"[^\\u0009-\\u000d\\u0020\\u0085\\u00a0\\u1680\\u2000-\\u200a\\u2028\\u2029\\u202f\\u205f\\u3000]"+"\""}];
return false;
}
}
}
}
else {
validate136.errors = [{instancePath:instancePath+"/name",schemaPath:"#/properties/name/type",keyword:"type",params:{type: "string"},message:"must be string"}];
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
if(!(validate95(data.settings, {instancePath:instancePath+"/settings",parentData:data,parentDataProperty:"settings",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate95.errors : vErrors.concat(validate95.errors);
errors = vErrors.length;
}
var valid0 = _errs4 === errors;
if(valid0){
if(data.workspace_version !== undefined && func0.call(data, "workspace_version")){
let data2 = data.workspace_version;
const _errs5 = errors;
if(typeof data2 !== "string"){
validate136.errors = [{instancePath:instancePath+"/workspace_version",schemaPath:"#/properties/workspace_version/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
if("umbra-observer-workspace-v1" !== data2){
validate136.errors = [{instancePath:instancePath+"/workspace_version",schemaPath:"#/properties/workspace_version/const",keyword:"const",params:{allowedValue: "umbra-observer-workspace-v1"},message:"must be equal to constant"}];
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
validate136.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate136.errors = vErrors;
return errors === 0;
}
validate136.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};


function validate94(data, {instancePath="", parentData, parentDataProperty, rootData=data, dynamicAnchors={}}={}){
/*# sourceURL="urn:umbra:observer:workspace-library:v1" */;
let vErrors = null;
let errors = 0;
const evaluated0 = validate94.evaluated;
if(evaluated0.dynamicProps){
evaluated0.props = undefined;
}
if(evaluated0.dynamicItems){
evaluated0.items = undefined;
}
if(errors === 0){
if(data && typeof data == "object" && !Array.isArray(data)){
if(data.current === undefined){
data.current = {"alerts":{"search":"","status":"all"},"declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"events":{"family":"","federate":"","search":"","source":"","type":""},"federate_declarations":{"direction":"all","kind":"all","perspective":"all","search":"","status":"all"},"federate_events":{"family":"","federate":"","search":"","source":"","type":""},"federate_time":{"grants_only":false,"selected":null},"federate_views":[],"federates":{"basis":"all","search":""},"federation":{"distribution":"event_types"},"fom":{"kind":"all","model":"all","search":"","selected":null,"status":"all"},"interactions":{"order":"all","search":"","status":"all"},"mom":{"counter":null,"scope":"all","search":"","service_status":"all"},"object_charts":[],"objects":{"scope":null,"search":"","selected":"","status":"all"},"ownership":{"owner_state":"all","request_status":"all","scope":"all","search":""},"persistence":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"regions":{"evidence":"all","kind":"all","search":"","selected":null,"status":"all"},"selected_federate":"","synchronization":{"kind":"all","phase":"all","scope":"all","search":"","selected":"all"},"time":{"grants_only":false,"selected":null},"topology":{"evidence":"all","focused":"all","kind":"all","search":"","source":"all","status":"all"},"view":"federation","window_seconds":300};
}
if(data.saved === undefined){
data.saved = [];
}
let missing0;
if(((data.library_version === undefined) || (!(func0.call(data, "library_version")))) && (missing0 = "library_version")){
validate94.errors = [{instancePath,schemaPath:"#/required",keyword:"required",params:{missingProperty: missing0},message:"must have required property '"+missing0+"'"}];
return false;
}
else {
const _errs1 = errors;
for(const key0 of Object.keys(data)){
if(!(((key0 === "current") || (key0 === "library_version")) || (key0 === "saved"))){
validate94.errors = [{instancePath,schemaPath:"#/additionalProperties",keyword:"additionalProperties",params:{additionalProperty: key0},message:"must NOT have additional properties"}];
return false;
break;
}
}
if(_errs1 === errors){
const _errs2 = errors;
if(!(validate95(data.current, {instancePath:instancePath+"/current",parentData:data,parentDataProperty:"current",rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate95.errors : vErrors.concat(validate95.errors);
errors = vErrors.length;
}
var valid0 = _errs2 === errors;
if(valid0){
if(data.library_version !== undefined && func0.call(data, "library_version")){
let data1 = data.library_version;
const _errs3 = errors;
if(typeof data1 !== "string"){
validate94.errors = [{instancePath:instancePath+"/library_version",schemaPath:"#/properties/library_version/type",keyword:"type",params:{type: "string"},message:"must be string"}];
return false;
}
if("umbra-observer-workspace-library-v1" !== data1){
validate94.errors = [{instancePath:instancePath+"/library_version",schemaPath:"#/properties/library_version/const",keyword:"const",params:{allowedValue: "umbra-observer-workspace-library-v1"},message:"must be equal to constant"}];
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
validate94.errors = [{instancePath:instancePath+"/saved",schemaPath:"#/properties/saved/maxItems",keyword:"maxItems",params:{limit: 12},message:"must NOT have more than 12 items"}];
return false;
}
else {
var valid1 = true;
const len0 = data2.length;
for(let i0=0; i0<len0; i0++){
const _errs7 = errors;
if(!(validate136(data2[i0], {instancePath:instancePath+"/saved/" + i0,parentData:data2,parentDataProperty:i0,rootData,dynamicAnchors}))){
vErrors = vErrors === null ? validate136.errors : vErrors.concat(validate136.errors);
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
validate94.errors = [{instancePath:instancePath+"/saved",schemaPath:"#/properties/saved/type",keyword:"type",params:{type: "array"},message:"must be array"}];
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
validate94.errors = [{instancePath,schemaPath:"#/type",keyword:"type",params:{type: "object"},message:"must be object"}];
return false;
}
}
validate94.errors = vErrors;
return errors === 0;
}
validate94.evaluated = {"props":true,"dynamicProps":false,"dynamicItems":false};
