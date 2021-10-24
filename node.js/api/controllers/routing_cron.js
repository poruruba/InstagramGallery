'use strict';

const THIS_BASE_PATH = process.env.THIS_BASE_PATH;
const CONTROLLERS_BASE = THIS_BASE_PATH + '/api/controllers/';

const CRON_TARGET_FNAME = "cron.json";
const DEFAULT_HANDLER = "handler";

const fs = require('fs');
const cron = require('node-cron');

function parse_cron() {
  // cron.jsonの検索
  const folders = fs.readdirSync(CONTROLLERS_BASE);
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

      // cronの登録
      const defs = JSON.parse(fs.readFileSync(fname).toString());
      defs.forEach(item => {
        if (!item.enable )
          return;
          
        const handler = item.handler || DEFAULT_HANDLER;
        const proc = require('./' + folder)[handler];

        cron.schedule(item.schedule, () => proc(item.param));
        console.log(item.schedule + " cron " + folder + ' ' + handler);
      });
    } catch (error) {
      console.log(error);
    }
  });
}

module.exports = parse_cron();
