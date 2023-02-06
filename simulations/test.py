from DataTypes import *
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

@app.route('/link/<car>/<fob>')
def link(car, fob):
    c = cars.get(car)
    f = fobs.get(fob)
    pass

if __name__ == '__main__':
    app.run(debug=True)
