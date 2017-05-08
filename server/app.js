const express = require('express')
const bodyParser = require('body-parser')
const morgan = require('morgan')

const app = express()

app.use(morgan('dev'))
app.use(bodyParser.json())
// eventually....
app.post('/v1/batch', (req, res) => {
  res.status(501)
})
app.post('/v1/:type', (req, res) => {
  res.json(req.body)
})

app.listen(55005, err => {
  if (err) {
    throw err
  }

  console.log(' > listening at http://localhost:55005/')
})
