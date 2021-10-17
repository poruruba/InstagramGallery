'use strict';

const THIS_BASE_PATH = process.env.THIS_BASE_PATH;

const HELPER_BASE = process.env.HELPER_BASE || '../../helpers/';
const Response = require(HELPER_BASE + 'response');
const TextResponse = require(HELPER_BASE + 'textresponse');

const DEFAULT_HANDLER = "handler";

const swagger_utils = require(HELPER_BASE + 'swagger_utils');
const { parse } = require('graphql');
const fs = require('fs');

const SWAGGER_FILE = THIS_BASE_PATH + "/api/swagger/swagger.yaml";
const CONTROLLERS_BASE = THIS_BASE_PATH + '/api/controllers/';
const CRON_TARGET_FNAME = "cron.json";
const MQTT_TARGET_FNAME = "mqtt.json";
const SWAGGER_TARGET_FNAME = "swagger.yaml";
const GRAPHQL_TARGET_FNAME = "schema.graphql";

exports.handler = async (event, context, callback) => {
  if( event.path == '/swagger'){
    const root_file = fs.readFileSync(SWAGGER_FILE, 'utf-8');
    const root = swagger_utils.parse_document(root_file);

    root.contents.set("host", event.headers.host);
    root.contents.set("basePath", event.stage);

    swagger_utils.delete_paths(root);
    swagger_utils.delete_definitions(root);

    const files = fs.readdirSync(CONTROLLERS_BASE);
    files.forEach(item => {
      const stats_dir = fs.statSync(CONTROLLERS_BASE + item);
      if (!stats_dir.isDirectory())
        return;
      try {
        fs.statSync(CONTROLLERS_BASE + item + '/' + SWAGGER_TARGET_FNAME);
      } catch (error) {
        return;
      }

      const file = fs.readFileSync(CONTROLLERS_BASE + item + '/' + SWAGGER_TARGET_FNAME, 'utf-8');
      const doc = swagger_utils.parse_document(file);

      swagger_utils.append_paths(root, doc, item);
      swagger_utils.append_definitions(root, doc, item);
    });

    return new Response(root);
  }else
  if( event.path == '/cron' ){
    const folders = fs.readdirSync(CONTROLLERS_BASE);
    let root = [];
    folders.forEach(folder => {
      if (!fs.existsSync(CONTROLLERS_BASE + folder))
        return;
      const stats_dir = fs.statSync(CONTROLLERS_BASE + folder);
      if (!stats_dir.isDirectory())
        return;

      try {
        const fname = CONTROLLERS_BASE + folder + "/" + CRON_TARGET_FNAME;
        if (!fs.existsSync(fname))
          return;
        const stats_file = fs.statSync(fname);
        if (!stats_file.isFile())
          return;

        const defs = JSON.parse(fs.readFileSync(fname).toString());
        const item = {
          operationId: folder,
          schedule: defs.schedule,
          handler: defs.handler ? defs.handler : DEFAULT_HANDLER,
          enable: defs.enable ? true : false
        };
        root.push(item);
      } catch (error) {
        console.log(error);
      }
    });

    return new Response(root);
  }else
  if( event.path == '/mqtt' ){
    const folders = fs.readdirSync(CONTROLLERS_BASE);
    let root = [];
    folders.forEach(folder => {
      if (!fs.existsSync(CONTROLLERS_BASE + folder))
        return;
      const stats_dir = fs.statSync(CONTROLLERS_BASE + folder);
      if (!stats_dir.isDirectory())
        return;

      try {
        const fname = CONTROLLERS_BASE + folder + "/" + MQTT_TARGET_FNAME;
        if (!fs.existsSync(fname))
          return;
        const stats_file = fs.statSync(fname);
        if (!stats_file.isFile())
          return;

        const defs = JSON.parse(fs.readFileSync(fname).toString());
        const item = {
          operationId: folder,
          topic: defs.topic,
          handler: defs.handler ? defs.handler : DEFAULT_HANDLER,
          enable: defs.enable ? true : false
        };
        root.push(item);
      } catch (error) {
        console.log(error);
      }
    });

    return new Response(root);
  }else
  if( event.path == '/graphql' ){
    let graphql_list = [];

    // schema.graphqlの検索
    const folders = fs.readdirSync(CONTROLLERS_BASE);
    folders.forEach(folder => {
      if (!fs.existsSync(CONTROLLERS_BASE + folder))
        return;
      const stats_dir = fs.statSync(CONTROLLERS_BASE + folder);
      if (!stats_dir.isDirectory())
        return;

      try {
        const fname = CONTROLLERS_BASE + folder + "/" + GRAPHQL_TARGET_FNAME;
        if (!fs.existsSync(fname))
          return;
        const stats_file = fs.statSync(fname);
        if (!stats_file.isFile())
          return;

        const typeDefs = fs.readFileSync(fname).toString();

        // schema.graphqlの解析
        const gqldoc = parse(typeDefs);
        //      console.log(JSON.stringify(gqldoc, null, 2));

        let endpoint = "/" + folder; // default endpoint
        gqldoc.definitions.forEach(element1 => {
          if (element1.kind == 'SchemaDefinition') {
            // endpoint(Schema部)の解析
            const h1 = element1.directives.find(item => item.name.value == 'endpoint');
            if (h1) {
              const h2 = h1.arguments.find(item => item.name.value == 'endpoint');
              if (h2) {
                endpoint = h2.value.value;
              }
            }
            return;
          }

        });

        graphql_list.push({
          operationId: folder,
          endpoint: endpoint
        });
      } catch (error) {
        console.log(error);
      }
    });

    let html = "<h1>graphql explorer</h1>";
    graphql_list.map(item => {
      html += `<a href='..${item.endpoint}'>${item.operationId}</a><br>`;
    });
    return new TextResponse("text/html", html);
  }
}
