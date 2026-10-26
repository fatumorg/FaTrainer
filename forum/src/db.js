const { Pool } = require("pg");

const createPool = (db) => new Pool({ ...db, options: "-c search_path=fatrainer,public", max: 10 });

module.exports = { createPool };
