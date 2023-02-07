from DataTypes import *
import json
import base64
from flask import Flask, render_template, request, redirect

app = Flask(__name__)

cars = {}
fobs = {}

factory = Factory()

@app.route('/')
def index():
    return render_template('index.html', car=str(cars), fob=str(fobs)) 

@app.route('/debug/add_paired', methods=['POST'])
def addPaired():
    if request.method == 'POST':
        key = request.form['key']
        if (not 'c_'+key in cars.keys() and not 'f_'+key in fobs.keys()):
            cars['c_'+key], fobs['f_'+key] = factory.gen_car_fob(random.randint(1, 0xffff))
    return redirect('/')
@app.route('/debug/add_unpaired', methods=['POST'])
def addUnpaired():
    if request.method == 'POST':
        print(request.form.keys())
        key = 'f_' + request.form['key']
        if (not key in cars.keys() and not key in fobs.keys()):
            fobs[key] = factory.gen_unpaired_fob()
    return redirect('/')

@app.route('/debug/reset', methods=['POST'])
def reset():
    cars = {}
    fobs = {}
    return redirect('/')

@app.route('/static/<car>/<fob>')
def burp(car, fob):
    b = json.dumps(fobs[fob].unlock().jsonify()).encode()
    return render_template('burp.html', j=(b.decode()), url=f'/link/{car}/{fob}')

@app.route('/send/<dest>', methods=['POST'])
def send(dest):
    out = ''
    if dest[:2] == 'c_':
        out = cars[dest]
    else:
        out = fobs[dest]
    msg = request.json
    out = json.dumps(out.on_message(Message.unjsonify(msg)).jsonify()).encode()

    return out.decode()

if __name__ == '__main__':
    app.run(debug=True)
